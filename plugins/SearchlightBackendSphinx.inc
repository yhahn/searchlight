<?php

define('SEARCHLIGHT_SPHINX_FILTER_MAX', 100000000000);
define('SEARCHLIGHT_SPHINX_FILTER_MIN', 0);


class SearchlightBackendSphinx extends SearchlightBackend {
  /**
   * Override of settingsDefault().
   */
  function settingsDefault() {
    return array(
      'port' => 9312,
      'delta_ttl' => 0,
      'sql_sock' => '',
    );
  }

  /**
   * Override of settingsForm().
   */
  function settingsForm() {
    $form = array();
    $form['sql_sock'] = array(
      '#title' => t('DB sock'),
      '#type' => 'textfield',
      '#default_value' => $this->settings['sql_sock'],
      '#description' => t('The DB sock file to use. Example: <code>/var/lib/mysql/mysql.sock</code>. If left empty, Sphinx will try to autodetect its location.'),
      '#size' => 30,
    );
    $form['port'] = array(
      '#title' => t('searchd port'),
      '#type' => 'textfield',
      '#default_value' => $this->settings['port'],
      '#size' => 6,
    );
    $form['delta_ttl'] = array(
      '#title' => t('Delta index TTL'),
      '#description' => t('In seconds. Set to "0" to re-index on every run.'),
      '#type' => 'textfield',
      '#default_value' => $this->settings['delta_ttl'],
      '#size' => 6,
    );
    return $form;
  }

  /**
   * Override of viewsOptionDefinition().
   */
  function viewsOptionDefinition(&$handler) {
    $this->sphinxIncludeClient();
    $options = parent::viewsOptionDefinition($handler);
    $options['matchMode'] = array('default' => SPH_MATCH_ALL);
    return $options;
  }

  /**
   * Override of viewsOptionsForm().
   */
  function viewsOptionsForm(&$form, &$form_state, &$handler) {
    $this->sphinxIncludeClient();
    parent::viewsOptionsForm($form, $form_state, $handler);
    $form['matchMode'] = array(
      '#title' => t('Search mode'),
      '#type' => 'select',
      '#options' => array(
        SPH_MATCH_ALL => t('Match all query words'),
        SPH_MATCH_ANY => t('Match any of the query words'),
        SPH_MATCH_PHRASE => t('Match the exact phrase'),
      ),
      '#default_value' => $handler->options['matchMode'],
    );
  }

  /**
   * Override of initClient().
   */
  function initClient($datasource) {
    $this->sphinxIncludeClient();
    $client = new SphinxClient();
    $client->SetServer('localhost', (int) $this->settings['port']);
    return $client;
  }

  /**
   * Override of executeQuery().
   */
  function executeQuery(&$client, $datasource, $query = '') {
    $result = $client->Query($query, $this->sphinxGetIndex($datasource));
    if (!empty($result['matches'])) {
      return array('result' => array_keys($result['matches']), 'total' => $result['total'], 'raw' => $result);
    }
    return FALSE;
  }

  /**
   * Override of facetBuild().
   */
  function facetBuild(&$client, $datasource, $query = '', $facets) {
    $client = drupal_clone($client);
    foreach ($facets as $facet) {
      $field = $datasource->fields[$facet['field']];
      $limit = $facet['limit'];

      switch ($field['datatype']) {
        case 'timestamp':
          $groupby = array('day' => SPH_GROUPBY_DAY, 'month' => SPH_GROUPBY_MONTH, 'year' => SPH_GROUPBY_YEAR);
          $granularity = !empty($facet['granularity']) ? $facet['granularity'] : 'month';
          $client->SetGroupBy($field['name'], $groupby[$granularity], '@group desc');
          $client->SetArrayResult(FALSE);
          break;
        default:
          $client->SetGroupBy($field['name'], SPH_GROUPBY_ATTR, '@count desc');
          $client->SetArrayResult($field['usage'] === 'multivalue');
          break;
      }

      // Set result limit.
      $client->SetLimits(0, (int) $limit);
      $client->AddQuery($query, $this->sphinxGetIndex($datasource));
    }

    // Execute queries.
    $results = $client->RunQueries();

    // Build facet results.
    $built = array();
    foreach (array_values($facets) as $num => $facet) {
      if (!empty($results[$num]['matches'])) {
        $field = $datasource->fields[$facet['field']];
        foreach (array_keys($results[$num]['matches']) as $id) {
          $attr = $results[$num]['matches'][$id]['attrs'];
          if (isset($attr['@groupby'], $attr['@count'])) {
            switch ($field['datatype']) {
              case 'text':
                $id = $this->sphinxGetOrdinal($datasource, $field['name'], $attr['@groupby']);
                break;
              case 'timestamp':
                $id = $this->sphinxGetTimestamp($attr['@groupby'], !empty($facet['granularity']) ? $facet['granularity'] : 'month');
                break;
              default:
                $id = $attr['@groupby'];
                break;
            }
            $built[$facet['field']][$id] = array('id' => $id, 'count' => $attr['@count']);
          }
        }
      }
    }
    return $built;
  }

  /**
   * Override of invalidateIndex().
   * Since Sphinx indexes can't be fully deleted from within Drupal, we mark
   * all items as deleted instead.
   */
  function invalidateIndex($datasource) {
    variable_del('searchlight_sphinx_last');
    $views_data = views_fetch_data($datasource->base_table);
    $base_field = $views_data['table']['base']['field'];

    $delete = array();
    $result = db_query("SELECT " . $base_field . " FROM {" . $datasource->base_table . "}");
    while ($row = db_fetch_object($result)) {
      $delete[$row->{$base_field}] = array(1);
    }

    $client = $this->initClient($datasource);
    $client->UpdateAttributes($datasource->id, array('searchlight_deleted'), $delete);
    $client->UpdateAttributes($datasource->id .'_delta', array('searchlight_deleted'), $delete);
  }

  /**
   * Override of setFilter().
   */
  function setFilter(&$client, $datasource, $filters) {
    // Hardwired delete filter. See sphinxDatasourceConf() for how this is
    // added to the sphinx index.
    $filters['searchlight_deleted'] = array(
      'field' => 'searchlight_deleted',
      'operator' => '=',
      'args' => array(0),
    );
    if (!empty($filters)) {
      // Iterate through once to separate out range filters.
      $range_filters = array();
      foreach ($filters as $key => $params) {
        $field    = $params['field'];
        $operator = $params['operator'];
        $args     = $params['args'];
        if (in_array($operator, array('<', '>', '>=', '<='))) {
          unset($filters[$key]);
          switch ($operator) {
            case '>':
            case '>=':
              $range_filters[$field]['field'] = $field;
              $range_filters[$field]['max'] = isset($range_filters[$field]['max']) ? $range_filters[$field]['max'] : SEARCHLIGHT_SPHINX_FILTER_MAX;
              $range_filters[$field]['min'] = eval("return {$args[0]};");
              $range_filters[$field]['exclude'] = $operator === '>' ? TRUE : FALSE;
              break;
            case '<':
            case '<=':
              $range_filters[$field]['field'] = $field;
              $range_filters[$field]['max'] = eval("return {$args[0]};");
              $range_filters[$field]['min'] = isset($range_filters[$field]['min']) ? $range_filters[$field]['min'] : SEARCHLIGHT_SPHINX_FILTER_MIN;
              $range_filters[$field]['exclude'] = $operator === '<' ? TRUE : FALSE;
              break;
            default:
              break;
          }
        }
      }
      // Now set range filters.
      foreach ($range_filters as $filter) {
        // The exclude option on range filters appears to be broken in
        // Sphinx 0.9.9. Update this code to support either operator once the
        // problem is fixed upstream.
        // $client->SetFilterRange($filter['field'], $filter['min'], $filter['max'], $filter['exclude']);
        $client->SetFilterRange($filter['field'], $filter['min'], $filter['max'], FALSE);
      }
      // Equality & set filters filters.
      foreach ($filters as $params) {
        $field    = $params['field'];
        $operator = $params['operator'];
        $args     = $params['args'];
        switch ($operator) {
          case '=':
            $this->sphinxSetFilter($client, $datasource, $field, $args, FALSE);
            break;
          case 'IN':
            $this->sphinxSetFilter($client, $datasource, $field, $args, FALSE);
            break;
          case 'NOT IN':
            $this->sphinxSetFilter($client, $datasource, $field, $args, TRUE);
            break;
        }
      }
    }
  }

  /**
   * Override of setOptions().
   */
  function setOptions(&$client, $datasource, $options) {
    if (isset($options['matchMode'])) {
      $client->SetMatchMode($options['matchMode']);
    }
  }

  /**
   * Override of setSort().
   */
  function setSort(&$client, $datasource, $sorts) {
    if (!empty($sorts)) {
      $sphinx_sorts = array();
      foreach ($sorts as $sort) {
        if ($sort['field'] === 'searchlight_weight') {
          $sphinx_sorts[] = "@weight {$sort['order']}";
        }
        else {
          $sphinx_sorts[] = "{$sort['field']} {$sort['order']}";
        }
      }
      $client->setSortMode(SPH_SORT_EXTENDED, implode(', ', $sphinx_sorts));
    }
  }

  /**
   * Override of setPager().
   */
  function setPager(&$client, $offset, $limit) {
    $limit = !empty($limit) ? $limit : 1000;
    if (!empty($offset) || !empty($limit)) {
      $client->SetLimits((int) $offset, (int) $limit);
    }
  }

  /**
   * Override of setNodeAccess().
   * For Sphinx API usage of filter ORs, see:
   * - http://www.sphinxsearch.com/faq.html#filter-or
   * - http://www.sphinxsearch.com/forum/view.html?id=3440
   */
  function setNodeAccess(&$client, $grants) {
    if (!empty($grants)) {
      $grants_sql = array();
      foreach ($grants as $realm => $gids) {
        foreach ($gids as $gid) {
          $grants_sql[] = "IF(IN(node_access_gid_{$realm}, {$gid}), 1, 0)";
        }
      }
      if (count($grants_sql)) {
        $grants_sql = implode('+', $grants_sql);
        $client->SetSelect("*, ({$grants_sql}) AS sphinx_node_access");
        $client->SetFilterRange('sphinx_node_access', 1, 1000);
      }
    }
  }

  /**
   * Override of drushSearchd().
   */
  function drushSearchd() {
    $file_path = conf_path() .'/sphinx/log';
    if (file_check_directory($file_path, TRUE)) {
      drush_op('drush_shell_exec', 'searchd --nodetach -c ' . drush_locate_root() .'/'. conf_path() . '/sphinx/sphinx.conf');
      return drush_log('Search daemon started.', 'success');
    }
    return drush_log("An error ocurred while starting the search daemon.", 'error');
  }

  /**
   * Override of drushIndex().
   */
  function drushIndex() {
    // Create the sphinx directory if it doesn't exist.
    $file_path = conf_path() .'/sphinx/indexes';
    if (file_check_directory($file_path, TRUE)) {
      // Determine if we should update deltas only.
      $delta = $this->settings['delta_ttl'] && (variable_get('searchlight_sphinx_last', 0) + $this->settings['delta_ttl']) > time();

      // Build list of this site's indexes.
      $indexes = array();
      $indexes_delta = array();
      foreach (searchlight_get_datasource() as $datasource) {
        $datasource->init();
        $indexes[$datasource->id] = $datasource->id;
        $indexes_delta[$datasource->id] = $datasource->id . '_delta';
      }

      $conf_file = drush_locate_root() .'/'. conf_path() . '/sphinx/sphinx.conf';

      drush_op('drush_shell_exec', 'indexer --rotate --config ' .$conf_file. ' ' . implode(' ', $delta ? $indexes_delta : $indexes));
      $output = drush_shell_exec_output();
      foreach ($output as $line) {
        drush_print($line);
      }

      // Take care of entity deletion by updating searchlight_delta to 1.
      // Note that we must update both the main and delta indices.
      foreach (searchlight_get_datasource() as $datasource) {
        $delete = array();
        $result = db_query("SELECT * FROM {searchlight_search} WHERE status = -1 AND type = '%s'", $datasource->base_table);
        while ($row = db_fetch_object($result)) {
          $delete[$row->id] = array(1);
        }
        if (!empty($update)) {
          $client = $this->initClient($datasource);
          $client->UpdateAttributes($indexes[$datasource->id], array('searchlight_deleted'), $delete);
          $client->UpdateAttributes($indexes_delta[$datasource->id], array('searchlight_deleted'), $delete);

          // Once updates are complete, remove the Drupal-side records.
          db_query("DELETE FROM {searchlight_search} WHERE status = -1 AND type = '%s'", $datasource->base_table);
        }
      }

      // Clear caches so that sphinx id -> facet name mapping is updated.
      drupal_flush_all_caches();
      db_query('UPDATE {searchlight_search} SET status = 1 WHERE status = 0');
      variable_set('searchlight_sphinx_last', time());
      return drush_log('Indexing complete.', 'success');
    }
    return drush_log("An error ocurred while indexing.", 'error');
  }

  /**
   * Override of drushConf().
   */
  function drushConf() {
   $file_path = conf_path();
   $conf_path = $file_path .'/sphinx.d';
    if (file_check_directory($conf_path, TRUE)) {
      // Collect configuration arrays for each datasource.
      foreach (searchlight_get_datasource() as $datasource) {
        $source = $this->sphinxDatasourceConf($datasource);

        // Add delta index.
        if (!empty($this->settings['delta_ttl'])) {
          $source = $this->sphinxDatasourceConf($source, TRUE);
        }

        $conf_file = "{$conf_path}/{$source['conf']['id']}.conf";

        // Generate configuration file from datasource.
        $sphinx_conf = theme('searchlight_sphinx_index_conf', $source);
        $existing = file_exists($conf_file) ? file_get_contents($conf_file) : '';
        if ($sphinx_conf !== $existing) {
          file_put_contents($conf_file, $sphinx_conf);
          drush_log("{$conf_file} was written successfully.", 'success');
        }
      }

      if (drush_get_option('sl--sphinx-base-conf', false)) {
        $conf_file = "{$file_path}/sphinx.conf";
        $sphinx_conf = theme('searchlight_sphinx_conf', $this->sphinxSearchdConf());
        if ($sphinx_conf) {
          $existing = file_exists($conf_file) ? file_get_contents($conf_file) : '';
          if ($sphinx_conf === $existing) {
            return drush_log("{$conf_file}/sphinx.conf is unchanged.", 'success');
          }
          else {
            file_put_contents($conf_file, $sphinx_conf);
            return drush_log("{$conf_file} was written successfully.", 'success');
          }
        }
      }
      return drush_log("Sphinx configuration files were written", 'success');

    }
    return drush_log("Sphinx configuration files could not be written.", 'error');
  }

  /**
   * Methods specific to the Sphinx backend ===================================
   */

  /**
   * Wrapper of $client->SetFilter().
   * Convert any string values to ordinals before passing through.
   */
  function sphinxSetFilter(&$client, $datasource, $attribute, $values, $exclude = FALSE) {
    $ordinals = array();
    foreach ($values as $arg) {
      $arg = trim($arg);
      if (is_numeric($arg)) {
        $ordinals[] = $arg;
      }
      else {
        $ordinal = $this->sphinxGetOrdinal($datasource, $attribute, $arg);
        if (is_numeric($ordinal)) {
          $ordinals[] = $ordinal;
        }
      }
    }
    return $client->SetFilter($attribute, $ordinals, $exclude);
  }

  /**
   * Generate an array representing the conf and index settings for a datasource.
   */
  function sphinxDatasourceConf($datasource, $delta = FALSE) {
    $datasource->init();
    $datasource_id = $delta ? $datasource->id . '_delta' : $datasource->id;

    $conf = array('conf' => array(), 'index' => array());

    // Retrieve db info.
    global $db_url, $db_type;
    $url = is_array($db_url) ? $db_url['default'] : $db_url;
    $url = parse_url($url);

    // Configuration options.
    $conf['conf']['id'] = $datasource_id;
    $conf['conf']['type'] = $db_type === 'mysqli' ? 'mysql' : $db_type;
    $conf['conf']['sql_user'] = urldecode($url['user']);
    $conf['conf']['sql_pass'] = isset($url['pass']) ? urldecode($url['pass']) : '';
    $conf['conf']['sql_host'] = urldecode($url['host']);
    $conf['conf']['sql_db']   = trim(urldecode($url['path']), '/');
    $conf['conf']['sql_port'] = isset($url['port']) ? urldecode($url['port']) : '3306';

    // Check for optional sql_sock option.
    $sock = trim($this->settings['sql_sock']);
    if (!empty($sock)) {
      $conf['conf']['sql_sock'] = $sock;
    }

    // Index information
    $conf['index']['path'] = drush_locate_root() .'/'. conf_path() . '/sphinx/indexes/' . $datasource_id;
    $conf['index']['docinfo'] = 'extern';
    $conf['index']['mlock'] = 0;
    $conf['index']['morphology'] = 'stem_en';
    $conf['index']['min_word_len'] = 1;
    $conf['index']['charset_type'] = 'utf-8';
    $conf['index']['html_strip'] = 0;

    // Build the query.
    $datasource->init();
    $datasource->view->build();
    if ($view = $datasource->view) {
      $sql_query = drupal_clone($view->query);
      $sql_query->add_where(0, "{$view->base_table}.{$view->base_field}" .' BETWEEN $start AND $end');
      $sql_query->add_field(NULL, '0', 'searchlight_deleted');
      if ($delta) {
        $this->queryFresh($sql_query);
      }
      $conf['conf']['sql_query'] = $this->sphinxWriteSQL($sql_query->query(), $sql_query->get_where_args());
      $conf['conf']['sql_query'] = str_replace("\n", " \\\n", trim($conf['conf']['sql_query']));

      // Build the info query.
      $sql_query_info = drupal_clone($view->query);
      $sql_query_info->add_where(0, "{$view->base_table}.{$view->base_field}" .' = $id');
      $sql_query_info->add_field(NULL, '0', 'searchlight_deleted');
      if ($delta) {
        $this->queryFresh($sql_query_info);
      }
      $conf['conf']['sql_query_info'] = $this->sphinxWriteSQL($sql_query_info->query(), $sql_query_info->get_where_args());
      $conf['conf']['sql_query_info'] = str_replace("\n", " \\\n", trim($conf['conf']['sql_query_info']));

      // Assume serial ids on the base table and step by 1000.
      $conf['conf']['sql_query_range'] = "SELECT MIN({$view->base_field}), MAX({$view->base_field}) FROM {$view->base_table}";
      $conf['conf']['sql_query_range'] = str_replace("\n", " \\\n", trim($conf['conf']['sql_query_range']));
      $conf['conf']['sql_range_step'] = 1000;
      $conf['conf']['sql_ranged_throttle'] = 0;

      // Merge in attributes.
      $sql_attr = array();
      $sphinx_type = array(
        'text'      => 'sql_attr_str2ordinal',
        'int'       => 'sql_attr_uint',
        'float'     => 'sql_attr_float',
        'timestamp' => 'sql_attr_timestamp',
      );

      // Hardwired delete flag.
      $sql_attr[] = "{$sphinx_type['int']} = searchlight_deleted";

      // Generate attribute entries.
      $fields = $datasource->fields;
      $fields = array_merge($fields, $datasource->buildNodeaccess());
      foreach ($fields as $field) {
        if ($field['usage'] === 'attribute' && isset($sphinx_type[$field['datatype']])) {
          $sql_attr[] = "{$sphinx_type[$field['datatype']]} = {$field['name']}";
        }
        // Generate multivalue queries.
        else if ($field['usage'] === 'multivalue' && $mva_view = searchlight_build_view($field['view'])) {
          $query = drupal_clone($mva_view->query);
          // Remove any fields that are not the id field or attribute field.
          foreach ($query->fields as $alias => $query_field) {
            if ($query_field['field'] === $mva_view->base_field && $query_field['table'] === $mva_view->base_table) {
              continue;
            }
            if ($query_field['field'] === $field['field']) {
              continue;
            }
            unset($query->fields[$alias]);
          }
          if ($delta) {
            $this->queryFresh($query);
          }
          $mva_query = $this->sphinxWriteSQL($query->query(), $query->get_where_args());
          $mva_query = str_replace("\n", " \\\n", trim($mva_query));
          $sql_attr[] = "sql_attr_multi = uint {$field['name']} from query; {$mva_query}";
        }
      }

      $sql_attr = array_unique($sql_attr);
      $conf['conf']['sql_attr'] = implode(" \n", $sql_attr);
    }

    return $conf;
  }

  /**
  * Get the Sphinx searchd settings.
  */
  function sphinxSearchdConf() {
    $searchd = array();
    $searchd['log'] = drush_locate_root() .'/'. conf_path() . '/sphinx/log/searchd.log';
    $searchd['query_log'] = drush_locate_root() .'/'. conf_path() . '/sphinx/log/query.log';
    $searchd['pid_file'] = drush_locate_root() .'/'. conf_path() . '/sphinx/log/searchd.pid';
    return $searchd;
  }

  /**
   * Include the Sphinx PHP API library.
   */
  function sphinxIncludeClient() {
    module_load_include('php', 'searchlight', 'libraries/sphinxapi');
  }

  /**
   * Write a SQL query with fully prefixed tables and replaced arguments.
   */
  function sphinxWriteSQL($query, $args) {
    _db_query_callback($args, TRUE);
    $query = db_prefix_tables($query);
    $query = preg_replace_callback(DB_QUERY_REGEXP, '_db_query_callback', $query);
    return $query;
  }

  /**
   * Get the ordinal integer for a given string, or vice versa.
   */
  function sphinxGetOrdinal($datasource, $attribute_name, $value, $reset = FALSE) {
    $datasource_id = $datasource->id;

    if (!isset($this->ordinals[$datasource_id]) || $reset) {
      $cid = "searchlight_sphinx_ordinals_{$datasource_id}";
      if (!$reset && $cache = cache_get($cid)) {
        $this->ordinals[$datasource_id] = $cache->data;
      }
      if ($reset || !isset($this->ordinals[$datasource_id])) {
        $mapping = $this->sphinxGetOrdinals($datasource);
        if (!empty($mapping)) {
          cache_set($cid, $mapping);
        }
        $this->ordinals[$datasource_id] = $mapping;
      }
    }
    if (is_numeric($value)) {
      return array_search($value, $this->ordinals[$datasource_id][$attribute_name]);
    }
    else {
      return isset($this->ordinals[$datasource_id][$attribute_name][$value]) ? $this->ordinals[$datasource_id][$attribute_name][$value] : FALSE;
    }
  }

  /**
   * Get an ordinal mapping for this datasource.
   * As this is a very expensive operation, its results are cached and
   * used in the ->sphinxGetOrdinal() method. You should only call this method
   * if you know what you are doing.
   */
  function sphinxGetOrdinals($datasource) {
    $datasource->init();

    // Retrieve all ordinal attributes.
    $attributes = array();
    if (!empty($datasource->fields)) {
      foreach ($datasource->fields as $name => $field) {
        if ($field['datatype'] === 'text' && $field['usage'] === 'attribute') {
          $attributes[$name] = $field;
        }
      }
    }

    // If there are attributes with ordinals, generate a mapping.
    // We only handle string ordinals for sets of less than 1000 (for now).
    $ordinals = array();
    if (!empty($attributes)) {
      $this->sphinxIncludeClient();
      $sphinx = new SphinxClient();
      $sphinx->SetLimits(0, 1000);

      foreach ($attributes as $name => $field) {
        // Dispatch a Sphinx query to retrieve the ordinal.
        $sphinx->SetGroupBy($name, SPH_GROUPBY_ATTR, '@count desc');
        $sphinx->SetGroupDistinct($name);
        $result = $sphinx->Query(NULL, $this->sphinxGetIndex($datasource));

        if (!empty($result['matches']) && count($result['matches']) < 1000) {
          // Dispatch a Views query to retrieve the corresponding string.
          $ids = implode(',', array_keys($result['matches']));
          $view = $datasource->view->copy();
          $view->build();
          $view->set_items_per_page(0);
          $view->query->where = array();
          $view->query->add_where(0, "{$view->base_table}.{$view->base_field} IN ({$ids})");
          $view->build_info['query'] = $view->query->query();
          $view->build_info['query_args'] = $view->query->get_where_args();
          $view->execute();
          foreach ($view->result as $row) {
            $id = $row->{$view->base_field};
            if (isset($result['matches'][$id], $row->{$name})) {
              $ordinals[$name][$row->{$name}] = $result['matches'][$id]['attrs'][$name];
            }
          }
        }
      }
    }
    return $ordinals;
  }

  /**
   * Convert a Sphinx datestring into a timestamp. Requires a datestring
   * (e.g. 201010) and a target granularity: day, month, year. We reverse-apply
   * Drupal's timezone offset so that this date can be used with format_date(),
   * etc. cleanly.
   */
  function sphinxGetTimestamp($datestring, $granularity) {
    global $user;
    if (variable_get('configurable_timezones', 1) && $user->uid && strlen($user->timezone)) {
      $timezone = $user->timezone;
    }
    else {
      $timezone = variable_get('date_default_timezone', 0);
    }
    switch ($granularity) {
      case 'day':
        return ($timezone * -1) + mktime(0, 0, 0, substr($datestring, 4, 2), substr($datestring, 6, 2), substr($datestring, 0, 4));
      case 'month':
        return ($timezone * -1) + mktime(0, 0, 0, substr($datestring, 4, 2), 1, substr($datestring, 0, 4));
      case 'year':
        return ($timezone * -1) + mktime(0, 0, 0, 1, 1, $datestring);
    }
  }

  /**
   * Return the index to query.
   */
  function sphinxGetIndex($datasource) {
    $index = $datasource->id;
    // Determine if we need to query a delta and main index.
    if (!empty($this->settings['delta_ttl'])) {
      $index .= ";{$index}_delta";
    }
    return $index;
  }
}
