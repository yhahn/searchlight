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
    );
  }

  /**
   * Override of settingsForm().
   */
  function settingsForm() {
    $form = array();
    $form['port'] = array(
      '#title' => t('searchd port'),
      '#type' => 'textfield',
      '#default_value' => $this->settings['port'],
      '#size' => 6,
    );
    return $form;
  }

  /**
   * Override of viewsOptionDefinition().
   */
  function viewsOptionDefinition(&$handler) {
    $this->includeSphinx();
    $options = parent::viewsOptionDefinition($handler);
    $options['matchMode'] = array('default' => SPH_MATCH_ALL);
    return $options;
  }

  /**
   * Override of viewsOptionsForm().
   */
  function viewsOptionsForm(&$form, &$form_state, &$handler) {
    $this->includeSphinx();
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
   * Override of queryInit().
   */
  function queryInit(&$searchlight) {
    $this->includeSphinx();
    $client = new SphinxClient();
    $client->SetServer('localhost', $this->settings['port']);
    $client->SetMatchMode($searchlight['options']['matchMode']);
    $searchlight['client'] = $client;
  }

  /**
   * Override of queryExecute().
   */
  function queryExecute(&$searchlight) {
    $searchlight['results'] = $searchlight['client']->Query($searchlight['query'], $searchlight['datasource_id']);
    return array(
      'result' => array_keys($searchlight['results']['matches']),
      'total_rows' => $searchlight['results']['total'],
    );
  }

  /**
   * Override of setFilter().
   */
  function setFilter(&$searchlight, $field, $operator, $args) {
    // If range filter has been requested, check for a previously added
    // rangefilter against the same field. If the two ranges overlap, use a
    // "between" range filter native to Sphinx instead.
    if (in_array($operator, array('<', '>', '>=', '<='))) {
      foreach ($searchlight['client']->_filters as $k => $filter) {
        if ($filter['attr'] === $field && in_array($filter['type'], array(SPH_FILTER_RANGE, SPH_FILTER_FLOATRANGE))) {
          unset($searchlight['client']->_filters[$k]);
          switch ($operator) {
            // Lessthan. We need to retain min and set the max.
            case '<':
            case '<=':
              $max = eval("return {$args[0]};");
              $searchlight['client']->SetFilterRange($field, $filter['min'], $max, FALSE);
              return TRUE;
            // Greaterthan. We need to retain max and set the min.
            case '>':
            case '>=':
              $min = eval("return {$args[0]};");
              $searchlight['client']->SetFilterRange($field, $min, $filter['max'], FALSE);
              return TRUE;
          }
        }
      }
    }
    // @TODO: between range filters (e.g. >= AND <=)
    switch ($operator) {
      case '=':
        $this->_SetFilter(&$searchlight, $field, $args, FALSE);
        return TRUE;
      case 'IN':
        $this->_SetFilter(&$searchlight, $field, $args, FALSE);
        return TRUE;
      case 'NOT IN':
        $this->_SetFilter(&$searchlight, $field, $args, TRUE);
        return TRUE;
      case '>':
        $max = SEARCHLIGHT_SPHINX_FILTER_MAX;
        $min = eval("return {$args[0]};");
        $searchlight['client']->SetFilterRange($field, $min, $max, TRUE);
        return TRUE;
      case '>=':
        $max = SEARCHLIGHT_SPHINX_FILTER_MAX;
        $min = eval("return {$args[0]};");
        $searchlight['client']->SetFilterRange($field, $min, $max, FALSE);
        return TRUE;
      case '<':
        $min = SEARCHLIGHT_SPHINX_FILTER_MIN;
        $max = eval("return {$args[0]};");
        $searchlight['client']->SetFilterRange($field, $min, $max, TRUE);
        return TRUE;
      case '<=':
        $min = SEARCHLIGHT_SPHINX_FILTER_MIN;
        $max = eval("return {$args[0]};");
        $searchlight['client']->SetFilterRange($field, $min, $max, FALSE);
        return TRUE;
    }
  }

  /**
   * Wrapper of $sphinx->SetFilter().
   * Convert any string values to ordinals before passing through.
   */
  function _SetFilter(&$searchlight, $attribute, $values, $exclude = FALSE) {
    $ordinals = array();
    foreach ($values as $arg) {
      $arg = trim($arg);
      if (is_numeric($arg)) {
        $ordinals[] = $arg;
      }
      else {
        $ordinal = $this->getOrdinal($searchlight, $attribute, $arg);
        if (is_numeric($ordinal)) {
          $ordinals[] = $ordinal;
        }
      }
    }
    return $searchlight['client']->SetFilter($attribute, $ordinals, $exclude);
  }

  /**
   * Override of setSort().
   */
  function setSort(&$searchlight, $field, $direction) {
    if (!isset($searchlight['sphinx_sort'])) {
      $searchlight['sphinx_sort'] = array();
    }
    // @TODO: Add support for SPH_SORT_TIME_SEGMENTS.
    if ($field === 'searchlight_weight') {
      $searchlight['sphinx_sort'][] = "@weight {$direction}";
    }
    else {
      $searchlight['sphinx_sort'][] = "{$field} {$direction}";
    }
    $searchlight['client']->setSortMode(SPH_SORT_EXTENDED, implode(', ', $searchlight['sphinx_sort']));
  }

  /**
   * Override of setPager().
   */
  function setPager(&$searchlight, $offset, $items_per_page) {
    // Max out at 1000 items.
    $items_per_page = !empty($items_per_page) ? $items_per_page : 1000;
    $searchlight['client']->SetLimits($offset, $items_per_page);
  }

  /**
   * Override of facetBuild().
   */
  function facetBuild(&$searchlight, $field) {
    // Make a copy for building this facet.
    $facet = $searchlight;

    // Init the Sphinx client. It may not exist if there has been no search
    // on this page.
    if (!isset($facet['client'])) {
      $this->queryInit($facet);
    }

    // @TODO: make configurable:
    // - # of items to show on a per facet basis
    switch ($field['datatype']) {
      case 'timestamp':
        $groupby = array('day' => SPH_GROUPBY_DAY, 'month' => SPH_GROUPBY_MONTH, 'year' => SPH_GROUPBY_YEAR);
        $granularity = !empty($field['granularity']) ? $field['granularity'] : 'month';
        $facet['client']->SetGroupBy($field['name'], $groupby[$granularity], '@group desc');
        break;
      default:
        $facet['client']->SetGroupBy($field['name'], SPH_GROUPBY_ATTR, '@count desc');
        $facet['client']->SetArrayResult($field['usage'] === 'multivalue');
        break;
    }
    $facet['client']->SetLimits(0, 5);
    $result = $this->queryExecute($facet);
    $items = array();
    foreach ($result['result'] as $id) {
      if (isset($facet['results']['matches'][$id]['attrs'])) {
        $attr = $facet['results']['matches'][$id]['attrs'];
        if (isset($attr['@groupby'], $attr['@count'])) {
          switch ($field['datatype']) {
            case 'text':
              $id = $this->getOrdinal($searchlight, $field['name'], $attr['@groupby']);
              break;
            case 'timestamp':
              $id = $this->getTimestamp($attr['@groupby'], !empty($field['granularity']) ? $field['granularity'] : 'month');
              break;
            default:
              $id = $attr['@groupby'];
              break;
          }
          $items[$id] = array('id' => $id, 'count' => $attr['@count']);
        }
      }
    }
    return $items;
  }

  function drushSearchd() {
    $file_path = conf_path() .'/sphinx/log';
    if (file_check_directory($file_path, TRUE)) {
      drush_op('drush_shell_exec', 'searchd --nodetach -c ' . drush_locate_root() .'/'. conf_path() . '/sphinx/sphinx.conf');
      return drush_log('Search daemon started.', 'success');
    }
    return drush_log("An error ocurred while starting the search daemon.", 'error');
  }

  function drushIndex() {
    // Create the sphinx directory if it doesn't exist.
    $file_path = conf_path() .'/sphinx/indexes';
    if (file_check_directory($file_path, TRUE)) {
      drush_op('drush_shell_exec', 'indexer --all --config ' . drush_locate_root() .'/'. conf_path() . '/sphinx/sphinx.conf');
      $output = drush_shell_exec_output();
      foreach ($output as $line) {
        drush_print($line);
      }
      drupal_flush_all_caches();
      return drush_log('Indexing complete.', 'success');
    }
    return drush_log("An error ocurred while indexing.", 'error');
  }

  function drushWriteConf() {
    $file_path = conf_path() .'/sphinx';
    if (file_check_directory($file_path, TRUE)) {
      // Collect configuration arrays for each datasource.
      $datasources = array();
      foreach (array_keys(variable_get('searchlight_views', array())) as $base_table) {
        if ($datasource = searchlight_get_datasource($base_table, TRUE)) {
          $datasources[$base_table] = $this->sphinxDatasourceConf($datasource);
        }
      }
      // Generate configuration file from datasources.
      $sphinx_conf = theme('searchlight_sphinx_conf', $datasources, $this->sphinxSearchdConf());
      if ($sphinx_conf) {
        $existing = file_exists("{$file_path}/sphinx.conf") ? file_get_contents("{$file_path}/sphinx.conf") : '';
        if ($sphinx_conf === $existing) {
          return drush_log("{$file_path}/sphinx.conf is unchanged.", 'success');
        }
        else {
          file_put_contents("{$file_path}/sphinx.conf", $sphinx_conf);
          return drush_log("{$file_path}/sphinx.conf was written successfully.", 'success');
        }
      }
    }
    return drush_log("{$file_path}/sphinx.conf could not be written.", 'error');
  }

  /**
   * Generate an array representing the conf and index settings for a datasource.
   */
  function sphinxDatasourceConf($datasource) {
    $conf = array('conf' => array(), 'index' => array());

    // Retrieve db info.
    global $db_url, $db_type;
    $url = is_array($db_url) ? $db_url['default'] : $db_url;
    $url = parse_url($url);

    // Configuration options.
    $conf['conf']['id'] = $datasource['id'];
    $conf['conf']['type'] = $db_type;
    $conf['conf']['sql_user'] = urldecode($url['user']);
    $conf['conf']['sql_pass'] = isset($url['pass']) ? urldecode($url['pass']) : '';
    $conf['conf']['sql_host'] = urldecode($url['host']);
    $conf['conf']['sql_db']   = trim(urldecode($url['path']), '/');
    $conf['conf']['sql_port'] = isset($url['port']) ? urldecode($url['port']) : '3306';

    // Build the query.
    if ($view = searchlight_build_view($datasource['view'])) {
      $sql_query = drupal_clone($view->query);
      $sql_query->add_where(0, "{$view->base_table}.{$view->base_field}" .' BETWEEN $start AND $end');
      $conf['conf']['sql_query'] = $this->writeQuery($sql_query->query(), $sql_query->get_where_args());
      $conf['conf']['sql_query'] = str_replace("\n", " \\\n", trim($conf['conf']['sql_query']));

      // Build the info query.
      $sql_query_info = drupal_clone($view->query);
      $sql_query_info->add_where(0, "{$view->base_table}.{$view->base_field}" .' = $id');
      $conf['conf']['sql_query_info'] = $this->writeQuery($sql_query_info->query(), $sql_query_info->get_where_args());
      $conf['conf']['sql_query_info'] = str_replace("\n", " \\\n", trim($conf['conf']['sql_query_info']));

      // Assume serial ids on the base table and step by 1000.
      $conf['conf']['sql_query_range'] = "SELECT MIN({$view->base_field}), MAX({$view->base_field}) FROM {$view->base_table}";
      $conf['conf']['sql_query_range'] = str_replace("\n", " \\\n", trim($conf['conf']['sql_query_range']));
      $conf['conf']['sql_range_step'] = 1000;
      $conf['conf']['sql_ranged_throttle'] = 0;
    }

    // Merge in attributes.
    $sql_attr = array();
    $sphinx_type = array(
      'text'      => 'sql_attr_str2ordinal',
      'int'       => 'sql_attr_uint',
      'float'     => 'sql_attr_float',
      'timestamp' => 'sql_attr_timestamp',
    );
    foreach ($datasource['fields'] as $field) {
      if ($field['usage'] === 'attribute' && isset($sphinx_type[$field['datatype']])) {
        $sql_attr[] = "{$sphinx_type[$field['datatype']]} = {$field['name']}";
      }
      // Generate multivalue queries.
      else if ($field['usage'] === 'multivalue' && $view = searchlight_build_view($field['view'])) {
        $query = drupal_clone($view->query);
        // Remove any fields that are not the id field or attribute field.
        foreach ($query->fields as $alias => $query_field) {
          if ($query_field['field'] === $view->base_field && $query_field['table'] === $view->base_table) {
            continue;
          }
          if ($query_field['field'] === $field['field']) {
            continue;
          }
          unset($query->fields[$alias]);
        }
        $mva_query = $this->writeQuery($query->query(), $query->get_where_args());
        $mva_query = str_replace("\n", " \\\n", trim($mva_query));
        $sql_attr[] = "sql_attr_multi = uint {$field['name']} from query; {$mva_query}";
      }
    }

    $sql_attr = array_unique($sql_attr);
    $conf['conf']['sql_attr'] = implode(" \n", $sql_attr);

    // Index information
    $conf['index']['path'] = drush_locate_root() .'/'. conf_path() . '/sphinx/indexes/' . $datasource['id'];
    $conf['index']['docinfo'] = 'extern';
    $conf['index']['mlock'] = 0;
    $conf['index']['morphology'] = 'stem_en';
    $conf['index']['min_word_len'] = 1;
    $conf['index']['charset_type'] = 'utf-8';
    $conf['index']['html_strip'] = 0;

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
  function includeSphinx() {
    module_load_include('php', 'searchlight', 'libraries/sphinxapi');
  }

  /**
   * Write a SQL query with fully prefixed tables and replaced arguments.
   */
  function writeQuery($query, $args) {
    _db_query_callback($args, TRUE);
    $query = db_prefix_tables($query);
    $query = preg_replace_callback(DB_QUERY_REGEXP, '_db_query_callback', $query);
    return $query;
  }

  /**
   * Get the ordinal integer for a given string, or vice versa.
   */
  function getOrdinal(&$searchlight, $attribute_name, $value, $reset = FALSE) {
    if (!isset($this->ordinals[$searchlight['base_table']]) || $reset) {
      $cid = "searchlight_sphinx_ordinals_{$searchlight['base_table']}";
      if (!$reset && $cache = cache_get($cid)) {
        $this->ordinals[$searchlight['base_table']] = $cache->data;
      }
      if ($reset || !isset($this->ordinals[$searchlight['base_table']])) {
        $mapping = $this->getOrdinals($searchlight['datasource']);
        if (!empty($mapping)) {
          cache_set($cid, $mapping);
        }
        $this->ordinals[$searchlight['base_table']] = $mapping;
      }
    }
    if (is_numeric($value)) {
      return array_search($value, $this->ordinals[$searchlight['base_table']][$attribute_name]);
    }
    else {
      return isset($this->ordinals[$searchlight['base_table']][$attribute_name][$value]) ? $this->ordinals[$searchlight['base_table']][$attribute_name][$value] : FALSE;
    }
  }

  /**
   * Get an ordinal mapping for this datasource.
   * As this is a very expensive operation, its results are cached and
   * used in the ->getOrdinal() method. You should only call this method
   * if you know what you are doing.
   */
  function getOrdinals($datasource) {
    // Retrieve all ordinal attributes.
    $attributes = array();
    if (!empty($datasource['fields'])) {
      foreach ($datasource['fields'] as $name => $field) {
        if ($field['datatype'] === 'text' && $field['usage'] === 'attribute') {
          $attributes[$name] = $field;
        }
      }
    }

    // If there are attributes with ordinals, generate a mapping.
    // We only handle string ordinals for sets of less than 1000 (for now).
    $ordinals = array();
    if (!empty($attributes)) {
      $this->includeSphinx();
      $sphinx = new SphinxClient();
      $sphinx->SetLimits(0, 1000);

      foreach ($attributes as $name => $field) {
        // Dispatch a Sphinx query to retrieve the ordinal.
        $sphinx->SetGroupBy($name, SPH_GROUPBY_ATTR, '@count desc');
        $sphinx->SetGroupDistinct($name);
        $result = $sphinx->Query(NULL, $datasource['id']);

        if (count($result['matches']) < 1000) {
          // Dispatch a Views query to retrieve the corresponding string.
          $ids = implode(',', array_keys($result['matches']));
          $view = searchlight_build_view($datasource['view']);
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
   * (e.g. 201010) and a target granularity: day, month, year.
   */
  function getTimestamp($datestring, $granularity) {
    switch ($granularity) {
      case 'day':
        return mktime(0, 0, 0, substr($datestring, 4, 2), substr($datestring, 6, 2), substr($datestring, 0, 4));
      case 'month':
        return mktime(0, 0, 0, substr($datestring, 4, 2), 1, substr($datestring, 0, 4));
      case 'year':
        return mktime(0, 0, 0, 1, 1, $datestring);
    }
  }
}
