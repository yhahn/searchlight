<?php

class SearchlightBackendSphinx extends SearchlightBackend {
  var $sphinxClient;
  // var $query;
  // var $params;
  // var $ordinals;

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

  function settingsDefault() {
    return array(
      'port' => 9312,
    );
  }

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
   * Override of options_form().
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
   * Execute a Sphinx query and provide its results to the View.
   */
  function queryInit(&$searchlight) {
    $this->includeSphinx();
    $client = new SphinxClient();
    $client->SetServer('localhost', $this->settings['port']);
    $client->SetMatchMode($searchlight['options']['matchMode']);
    $searchlight['client'] = $client;
  }

  function queryExecute(&$searchlight) {
    $searchlight['results'] = $searchlight['client']->Query($searchlight['query'], $searchlight['datasource_id']);
    return array(
      'result' => array_keys($searchlight['results']['matches']),
      'total_rows' => $searchlight['results']['total'],
    );
  }

  /**
   * Set a Sphinx filter from a SQL WHERE clause.
   */
  function setFilter(&$searchlight, $clause, &$where_args) {
    $datasource = sphinx_views_get_datasource($this->view->base_table);
    $split = preg_split('/[ ]([<>=!]*)|(IN)|(NOT IN)/i', trim($clause), NULL, PREG_SPLIT_NO_EMPTY | PREG_SPLIT_DELIM_CAPTURE);
    if (count($split) >= 2) {
      $field = trim($split[0]);
      $operator = trim($split[1]);
      if (!empty($datasource['attributes'][$field])) {
        $attribute = $datasource['attributes'][$field];

        // Get the arguments for this where clause.
        $args = array();
        $argnum = substr_count($split[2], '%');
        if ($argnum === 0) {
          $args[] = $split[2];
        }
        else if (count($where_args) >= $argnum) {
          while ($argnum > 0) {
            $args[] = array_shift($where_args);
            $argnum--;
          }
        }
        // @TODO: between range filters (e.g. >= AND <=)
        switch (strtoupper($operator)) {
          case '=':
            $this->SetFilter($attribute['name'], $args, FALSE);
            return TRUE;
          case 'IN':
            $this->SetFilter($attribute['name'], $args, FALSE);
            return TRUE;
          case 'NOT IN':
            $this->SetFilter($attribute['name'], $args, TRUE);
            return TRUE;
          case '>':
            $max = 10000000000;
            $min = eval("return {$args[0]};");
            $this->sphinx->SetFilterRange($attribute['name'], $min, $max, TRUE);
            return TRUE;
          case '>=':
            $max = 10000000000;
            $min = eval("return {$args[0]};");
            $this->sphinx->SetFilterRange($attribute['name'], $min, $max, FALSE);
            return TRUE;
          case '<':
            $min = 0;
            $max = eval("return {$args[0]};");
            $this->sphinx->SetFilterRange($attribute['name'], $min, $max, TRUE);
            return TRUE;
          case '<=':
            $min = 0;
            $max = eval("return {$args[0]};");
            $this->sphinx->SetFilterRange($attribute['name'], $min, $max, FALSE);
            return TRUE;
        }
      }
    }
    return FALSE;
  }

  /**
  * Set global pager vars based on the view.
  */
  function setPager(&$searchlight, $offset, $items_per_page) {
    // Max out at 1000 items.
    $items_per_page = !empty($items_per_page) ? $items_per_page : 1000;
    $searchlight['client']->SetLimits($offset, $items_per_page);
  }

  /**
   * Set Sphinx sort method from ORDER BY clauses.
   */
  function setSort(&$searchlight, $orderby = array()) {
    $sort = array();
    $datasource = $searchlight['datasource'];
    foreach ($orderby as $clause) {
      $split = explode(' ', $clause);
      if (count($split) <= 2) {
        $field = $split[0];
        $direction = isset($split[1]) ? $split[1] : 'ASC';
        if ($field === 'searchlight_weight') {
          $sort[] = "@weight {$direction}";
          continue;
        }
        foreach ($datasource['attributes'] as $attribute) {
          if ($attribute['name'] === $field) {
            $sort[] = "{$field} {$direction}";
            continue;
          }
        }
      }
    }
    // @TODO: Add support for SPH_SORT_TIME_SEGMENTS.
    if (!empty($sort)) {
      $searchlight['client']->setSortMode(SPH_SORT_EXTENDED, implode(', ', $sort));
    }
    else {
      $searchlight['client']->setSortMode(SPH_SORT_RELEVANCE);
    }
  }

  /**
   * Wrappers =================================================================
   */

  /**
   * Wrapper of $sphinx->SetFilter().
   * Convert any string values to ordinals before passing through.
   */
  /*
  function SetFilter($attribute, $values, $exclude = FALSE) {
    $ordinals = array();
    foreach ($values as $arg) {
      $arg = trim($arg);
      if (is_numeric($arg)) {
        $ordinals[] = $arg;
      }
      else {
        $ordinal = $this->get_ordinal($attribute, $arg);
        if (is_numeric($ordinal)) {
          $ordinals[] = $ordinal;
        }
      }
    }
    return $this->sphinx->SetFilter($attribute, $ordinals, $exclude);
  }
  */

  /**
   * Execute the current Sphinx search with a groupby on the given facet.
   */
  function execute_facet($attribute, $count = 20) {
    $this->sphinx->SetGroupBy($attribute, SPH_GROUPBY_ATTR, '@count desc');
    $this->sphinx->SetLimits(0, $count);
    return $this->execute_sphinx();
  }

  /**
   * Build a facet results ready for theming from a Sphinx query result
   * returned by ->execute_facet().
   */
  function build_facet($attribute, $result = array()) {
    $items = array();
    $ids = array();

    // Collect facet information from result set.
    if (!empty($result['matches'])) {
      foreach ($result['matches'] as $row) {
        if (isset($row['attrs']['@groupby'], $row['attrs']['@count'])) {
          if ($attribute['type'] === 'sql_attr_str2ordinal') {
            $id = $this->get_ordinal($attribute['name'], $row['attrs']['@groupby']);
          }
          else {
            $id = $row['attrs']['@groupby'];
          }
          $count = $row['attrs']['@count'];
          $ids[] = $id;
          $items[$id] = array(
            'id' => $id,
            'count' => $count,
            'attribute' => $attribute,
          );
        }
      }
    }

    // Render facets.
    return $this->render_facet($attribute, $items);
  }

  /**
   * Render the facet using its view handler.
   */
  function render_facet($attribute, &$items) {
    $rendered = array();
    if ($attribute['type'] === 'sql_attr_multi') {
      $view = views_get_view($attribute['view']);
      $view->set_display($attribute['display']);
      $view->build();

      $options = $view->execute_display();
      if (!empty($options['mva_label_field']) && !empty($view->field[$options['mva_label_field']])) {
        $handler = $view->field[$options['mva_label_field']];
        $handler_fields = array($handler->real_field);

        // Claim: The table of the label field contains both the MVA field and
        // the label field itself (and are sometimes the same thing).
        $ids = array_keys($items);
        $placeholders = is_numeric($ids[0]) ? db_placeholders($ids, 'int') : db_placeholders($ids, 'text');
        $query =
          "SELECT {$attribute['field']} AS {$attribute['name']}, {$handler->real_field} AS {$handler->field_alias}
          FROM {{$handler->table}} {$handler->table}
          WHERE {$handler->table}.{$attribute['field']} IN ($placeholders)";
        $result = db_query($query, $ids);

        // Now render each row.
        while ($row = db_fetch_object($result)) {
          $items[$row->{$attribute['name']}]['rendered'] = $handler->render($row);
        }
        $label = $handler->label();
      }
    }
    else {
      $view = sphinx_views_get_view($this->view->base_table);
      $view->build();

      // Detect which field handler generated this attribute. Real pain.
      if (isset($view->field[$attribute['field']])) {
        $handler = $view->field[$attribute['field']];
      }
      else {
        foreach ($view->field as $field) {
          if ($field->real_field === $attribute['field'] || (!empty($field->additional_fields) && in_array($attribute['field'], $field->additional_fields))) {
            $handler = $field;
            break;
          }
        }
      }

      // Fake some rows so we can use the pre_render method.
      if (!empty($handler)) {
        $rows = array();
        foreach (array_keys($items) as $id) {
          $row = new stdClass();
          $row->{$attribute['name']} = $id;
          $rows[] = $row;
        }
        $handler->pre_render($rows);

        // Now render each row.
        foreach ($rows as $row) {
          $items[$row->{$attribute['name']}]['rendered'] = $handler->render($row);
        }
        $label = $handler->label();
      }
    }
    return array('label' => $label, 'items' => $items);
  }

  /**
   * Get the ordinal integer for a given string, or vice versa.
   */
  function get_ordinal($attribute_name, $value, $reset = FALSE) {
    if (!isset($this->ordinals) || $reset) {
      $cid = "sphinx_ordinals_{$this->view->base_table}";
      if (!$reset && $cache = cache_get($cid)) {
        $this->ordinals = $cache->data;
      }
      if ($reset || !isset($this->ordinals)) {
        $mapping = $this->get_ordinals();
        if (!empty($mapping)) {
          cache_set($cid, $mapping);
        }
        $this->ordinals = $mapping;
      }
    }
    if (is_numeric($value)) {
      return array_search($value, $this->ordinals[$attribute_name]);
    }
    else {
      return isset($this->ordinals[$attribute_name][$value]) ? $this->ordinals[$attribute_name][$value] : FALSE;
    }
  }

  /**
   * Get an ordinal mapping for this datasource.
   * As this is a very expensive operation, its results are cached and
   * used in the ->get_ordinal() method. You should only call this method
   * if you know what you are doing.
   */
  function get_ordinals() {
    $datasource = sphinx_views_get_datasource($this->view->base_table);

    // Retrieve all ordinal attributes.
    $attributes = array();
    if (!empty($datasource['attributes'])) {
      foreach ($datasource['attributes'] as $field => $attribute) {
        if ($attribute['type'] === 'sql_attr_str2ordinal') {
          $attributes[$field] = $attribute;
        }
      }
    }

    // If there are attributes with ordinals, generate a mapping.
    // We only handle string ordinals for sets of less than 1000 (for now).
    $ordinals = array();
    if (!empty($attributes)) {
      $sphinx = new SphinxClient();
      $sphinx->SetLimits(0, 1000);

      foreach ($attributes as $attribute) {
        // Dispatch a Sphinx query to retrieve the ordinal.
        $sphinx->SetGroupBy($attribute['name'], SPH_GROUPBY_ATTR, '@count desc');
        $sphinx->SetGroupDistinct($attribute['name']);
        $result = $sphinx->Query(NULL, sphinx_views_get_datasource_id($this->view->base_table));

        if (count($result['matches']) < 1000) {
          // Dispatch a Views query to retrieve the corresponding string.
          $ids = implode(',', array_keys($result['matches']));
          $view = sphinx_views_get_view($this->view->base_table);
          $view->build();
          $view->set_items_per_page(0);
          $view->query->where = array();
          $view->query->add_where(0, "{$view->base_table}.{$view->base_field} IN ({$ids})");
          $view->build_info['query'] = $view->query->query();
          $view->build_info['query_args'] = $view->query->get_where_args();
          $view->execute();
          foreach ($view->result as $row) {
            $id = $row->{$view->base_field};
            if (isset($result['matches'][$id], $row->{$attribute['name']})) {
              $ordinals[$attribute['name']][$row->{$attribute['name']}] = $result['matches'][$id]['attrs'][$attribute['name']];
            }
          }
        }
      }
    }
    return $ordinals;
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
    $view = $datasource['view'];
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
//     // Remove any fields that are not the id field or attribute field.
//     $options = $this->get_option('searchlight_multivalue');
//     $query = drupal_clone($this->view->query);
//     foreach ($query->fields as $alias => $query_field) {
//       if ($query_field['field'] === $this->view->base_field && $query_field['table'] === $this->view->base_table) {
//         continue;
//       }
//       if ($query_field['field'] === $options['attribute']) {
//         $attribute = $alias;
//         $table = $query_field['table'];
//         $field = $query_field['field'];
//       }
//       else {
//         unset($query->fields[$alias]);
//       }
//     }
//
//     // Build MVA query.
//     return array(
//       'type' => 'sql_attr_multi',
//       'name' => $attribute,
//       'table' => $table,
//       'field' => $field,
//       'view' => $this->view->name,
//       'display' => $this->view->display_handler->display->id,
//       'mva_label_field' => $options['facet'],
//       'mva_type' => 'uint',
//       'mva_source' => 'query',
//       'mva_query' => $this->db_query($query->query(), $query->get_where_args()),
//     );

      // if ($attribute['type'] === 'sql_attr_multi') {
      //   $query = str_replace("\n", " \\\n", trim($attribute['mva_query']));
      //   $sql_attr[] = "{$attribute['type']} = {$attribute['mva_type']} {$attribute['name']} from {$attribute['mva_source']}; {$query}";
      // }
      // else {
      // }
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
}
