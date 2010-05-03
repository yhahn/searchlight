<?php

//define('SEARCHLIGHT_solr_FILTER_MAX', 100000000000);
//define('SEARCHLIGHT_solr_FILTER_MIN', 0);


class SearchlightBackendSolr extends SearchlightBackend {
  /**
   * Override of settingsDefault().
   */
  function settingsDefault() {
    return array(
      'host' => 'localhost',
      'port' => 8983,
      'path' => '/solr',
    );
  }

  /**
   * Override of settingsForm().
   */
  function settingsForm() {
    $form = array();
    $form['host'] = array(
      '#title' => t('Solr host'),
      '#type' => 'textfield',
      '#default_value' => $this->settings['host'],
      '#size' => 60,
    );
    $form['port'] = array(
      '#title' => t('Solr port'),
      '#type' => 'textfield',
      '#default_value' => $this->settings['port'],
      '#size' => 6,
    );
    $form['path'] = array(
      '#title' => t('Solr path'),
      '#type' => 'textfield',
      '#default_value' => $this->settings['path'],
      '#size' => 60,
    );
    return $form;
  }

  /**
   * Override of initClient().
   */
  function initClient($datasource) {
    $this->includeSolr();
    $path = $this->settings['path'] .'/'. $datasource->id;
    $client = new Apache_Solr_Service($this->settings['host'], (int) $this->settings['port'], $path);
    return $client;
  }

  /**
   * Override of executeQuery().
   */
  function executeQuery(&$client, $datasource, $query = '') {
    // TODO ping here?
    $base_field = $this->solrGetBaseField($datasource);
    
    $response = $client->search($query, 0, 10, array());
    foreach ($response->response->docs as $doc) {
      $ids[] = $doc->{$base_field}; // determine from base field
    }

    if (!empty($ids)) {
      return array(
        'result' => $ids, 
        'total' => $response->response->numFound,
        'raw' => $response,
      );
    }
    return FALSE;
  }

  /**
   * Override of setFilter().
   */
  function setFilter(&$client, $datasource, $filters) {
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
              $range_filters[$field]['max'] = isset($range_filters[$field]['max']) ? $range_filters[$field]['max'] : SEARCHLIGHT_solr_FILTER_MAX;
              $range_filters[$field]['min'] = eval("return {$args[0]};");
              $range_filters[$field]['exclude'] = $operator === '>' ? TRUE : FALSE;
              break;
            case '<':
            case '<=':
              $range_filters[$field]['field'] = $field;
              $range_filters[$field]['max'] = eval("return {$args[0]};");
              $range_filters[$field]['min'] = isset($range_filters[$field]['min']) ? $range_filters[$field]['min'] : SEARCHLIGHT_solr_FILTER_MIN;
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
        // solr 0.9.9. Update this code to support either operator once the
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
            $this->_SetFilter($client, $datasource, $field, $args, FALSE);
            break;
          case 'IN':
            $this->_SetFilter($client, $datasource, $field, $args, FALSE);
            break;
          case 'NOT IN':
            $this->_SetFilter($client, $datasource, $field, $args, TRUE);
            break;
        }
      }
    }
  }

  /**
   * Wrapper of $solr->SetFilter().
   * Convert any string values to ordinals before passing through.
   */
  function _SetFilter(&$client, $datasource, $attribute, $values, $exclude = FALSE) {
    $ordinals = array();
    foreach ($values as $arg) {
      $arg = trim($arg);
      if (is_numeric($arg)) {
        $ordinals[] = $arg;
      }
      else {
        $ordinal = $this->getOrdinal($datasource, $attribute, $arg);
        if (is_numeric($ordinal)) {
          $ordinals[] = $ordinal;
        }
      }
    }
    return $client->SetFilter($attribute, $ordinals, $exclude);
  }

  /**
   * Override of setPager().
   */
  function setPager(&$client, $offset, $limit) {
  }

  /**
   * Override of setNodeAccess().
   * For solr API usage of filter ORs, see:
   * - http://www.solrsearch.com/faq.html#filter-or
   * - http://www.solrsearch.com/forum/view.html?id=3440
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
        $client->SetSelect("*, ({$grants_sql}) AS solr_node_access");
        $client->SetFilterRange('solr_node_access', 1, 1000);
      }
    }
  }

  /**
   * Override of facetBuild().
   */
  function facetBuild($query, $field, $limit = 5) {
    switch ($field['datatype']) {
      case 'timestamp':
        $groupby = array('day' => SPH_GROUPBY_DAY, 'month' => SPH_GROUPBY_MONTH, 'year' => SPH_GROUPBY_YEAR);
        $granularity = !empty($field['granularity']) ? $field['granularity'] : 'month';
        $query->client->SetGroupBy($field['name'], $groupby[$granularity], '@group desc');
        break;
      default:
        $query->client->SetGroupBy($field['name'], SPH_GROUPBY_ATTR, '@count desc');
        $query->client->SetArrayResult($field['usage'] === 'multivalue');
        break;
    }

    // Set result limit.
    $query->client->SetLimits(0, (int) $limit);

    $result = $this->executeQuery($query->client, $query->datasource, $query->search_query);
    if ($result) {
      $items = array();
      foreach ($result['result'] as $id) {
        if (isset($result['raw']['matches'][$id]['attrs'])) {
          $attr = $result['raw']['matches'][$id]['attrs'];
          if (isset($attr['@groupby'], $attr['@count'])) {
            switch ($field['datatype']) {
              case 'text':
                $id = $this->getOrdinal($query->datasource, $field['name'], $attr['@groupby']);
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
    return FALSE;
  }

  function drushSearchd() {
    $jar_path = '/usr/local/apache-solr-1.4.0/example'; // TODO move to option.

    $file_path = conf_path() .'/solr';
    $solr_home = drush_locate_root() .'/'. conf_path() . '/solr';
    $log_dir = $solr_home .'/log';
    if (file_check_directory($log_dir, TRUE)) {
      $opts .= '-Dsolr.solr.home='. $solr_home .' ';
      $opts .= '-Djetty.logs='. $log_dir.' ';
      $opts .= '-Djetty.home='. $jar_path.' ';
      $opts .= '-jar '. $jar_path .'/start.jar';
      drush_op('drush_shell_exec', 'java '. $opts);
    }
    return drush_log("An error ocurred while starting the search daemon.", 'error');
  }

  function drushIndex() {
    // build list of this site's indexes
    $indexes = array();
    foreach (searchlight_datasource_load() as $datasource) {
      $datasource->setId()->viewInit();
      $datasource->view->new_display('searchlight_solr', 'Searchlight Solr', 'searchlight_solr');
      $datasource->view->set_display('searchlight_solr');
      $datasource->viewSetHandlers();
      $handler = $datasource->view->display_handler;

      $view_fields = $handler->options['fields'];
      foreach ($datasource->fields as $field) {
        if ($field['datatype'] == 'timestamp') {
          $view_fields[$field['name']]['date_format'] = 'custom';
          $view_fields[$field['name']]['custom_date_format'] = 'Y-m-d\\TH:i:s\\Z';
        }
      }
      $handler->override_option('fields', $view_fields);

      // Add sort
      $base_field = $this->solrAddBaseField($datasource);
      $sorts[$base_field] = array(
        'id' => $base_field,
        'table' => $datasource->base_table,
        'field' => $base_field,
      );
      $handler->override_option('sorts', $sorts);

      $datasource->view->build();
      // Build query object and add the "fresh" join.
      $datasource->view->built = false;
      $this->queryFresh($datasource->view->query);

      // TODO make limit configurable
      $datasource->view->set_items_per_page(100);

      $items = $datasource->view->render();
      if (!empty($items)) {
        $solr = $this->initClient($datasource->id);
        $documents = array();
        $ids = array();
        foreach($items as $item) {
          $document = new Apache_Solr_Document();
          $content = array();
          foreach ($datasource->fields as $field) {
            if ($field['usage'] == 'content') {
              $content[] = $this->apachesolr_clean_text($item[$field['name']]);
            }
            else {
              $document->{$field['name']} = $item[$field['name']];
            }
          }
          $ids[] = $item[$datasource->view->base_field];
          $document->_body = implode(' ', $content);
          $documents[] = $document;
        }

        $solr->addDocuments($documents);
        $solr->commit();
        // $solr->optimize(); //merges multiple segments into one

        db_query("UPDATE {searchlight_search} SET status = 1 WHERE type ='node'
                  AND id IN (". rtrim(str_repeat('%d,', count($ids)), ',') .")", $ids);
        return drush_log('Indexing complete.', 'success');
      }

      variable_set('searchlight_solr_last', time());
      return drush_log('Nothing to Index.', 'success');
    }
    return drush_log("An error ocurred while indexing.", 'error');
  }

  // From the apachsolr module
  function apachesolr_clean_text($text) {
    // Add spaces before stripping tags to avoid running words together.
    $text = filter_xss(str_replace(array('<', '>'), array(' <', '> '), $text), array());
    // Decode entities and then make safe any < or > characters.
    return htmlspecialchars(html_entity_decode($text, ENT_NOQUOTES, 'UTF-8'), ENT_NOQUOTES, 'UTF-8');
  }

  function drushWriteConf() {
    $file_path = conf_path() .'/solr';
    if (file_check_directory($file_path, TRUE)) {
      // Collect configuration arrays for each datasource.
      $cores = array();
      foreach (searchlight_datasource_load() as $datasource) {
        $datasource->init();
        $cores[] = $datasource->id;
        $core_path = $file_path .'/'. $datasource->id;
        if (!file_check_directory($core_path, TRUE)) {
          return drush_log("/{$core_path} could not be written to.", 'error');
        }
        $core_path .= '/conf';
        if (file_check_directory($core_path, TRUE)) {
          // Generate configuration file from datasources.
          $schema = $this->solrDatasourceConf($datasource);
          $files = array(
            'schema.xml' => theme('searchlight_solr_schema', $schema),
            'solrconfig.xml' => theme('searchlight_solr_config'),
          );
          $this->solrWriteFiles($core_path, $files);
        }
        else {
          return drush_log("/{$core_path} could not be written to.", 'error');
        }
      }
      // Generate top level config.
      $files = array('solr.xml' => theme('searchlight_solr_cores', $cores));
      $this->solrWriteFiles($file_path, $files);
    }
    else {
      return drush_log("/{$filepath} could not be written to.", 'error');
    }
  }

  /**
   * Generate an array representing the conf and index settings for a datasource.
   */
  function solrDatasourceConf($datasource) {

    // Configuration options.
    $conf = array(
      'conf' => array(
        'default_search_field' => '_body',
      ),
      'schema' => array(
        '_body' => array(
          'name' => '_body',
          'type' => 'text',
        ),
      ),
    );
    $conf['conf']['id'] = $datasource->id;
    $conf['conf']['unique_key'] = $this->solrAddBaseField($datasource);

    // Attributes: name, type, indexed, stored, required, multiValued, omitNorms
    foreach ($datasource->fields as $field) {
      if ($field['usage'] != 'content') {
        $f = $field['name'];
        $conf['schema'][$f] = array('name' => $f);
        switch ($field['datatype']) {
          case 'int':
            $conf['schema'][$f]['type'] = 'integer';
            break;
          case 'text':
            $conf['schema'][$f]['type'] = 'string';
            break;
          case 'timestamp':
            $conf['schema'][$f]['type'] = 'date';
        }

        if ($field['usage'] == 'multivalue') {
          $conf['schema'][$f]['multiValued'] = TRUE;
        }
      }
    }
    return $conf;
  }

  /**
   * Write config to the filesystem
   */
  function solrWriteFiles($path, $files) {
    foreach($files as $name => $contents) {
      if ($contents) {
        $existing = '';
        if (file_exists("{$path}/{$name}")) {
          $existing = file_get_contents("{$path}/{$name}");
        }
        if ($contents !== $existing) {
         file_put_contents("{$path}/{$name}", $contents);
         drush_log("{$path}/{$name} was written successfully.", 'success');
        }
        else {
          drush_log("{$path}/{$name} is unchanged.", 'success');
        }
      }
      else {
        drush_log("{$path}/{$name} was not generated.", 'warning');
      }
    }
  }

  /**
   * Include the solr PHP API library.
   */
  function includeSolr() {
    module_load_include('php', 'searchlight', 'libraries/SolrPhpClient/Apache/Solr/Service');
  }

  /**
   * Make sure the base field is on the $datasource, and return it's name.
   */
  function solrAddBaseField(&$datasource) {
    $base_field = $this->solrGetBaseField($datasource);

    // Determine the primary id field of the view & determine if it is present.
    foreach ($datasource->fields as $field) {
      if ($field['table'] == $datasource->base_table && $field['name'] == $base_field) {
        return $base_field;
      }
    }

    // Add the primary field if it isn't present
    $datasource->fields[] = array(
      'name' => $base_field,
      'datatype' => 'int',
    );
    return $base_field;
  }
  
  /**
   * Lookup and return the base field for a table.
   */
  function solrGetBaseField($datasource) {
    $base_table = views_fetch_data($datasource->base_table);
    return $base_table['table']['base']['field']; 
  }

  /**
   * Get the ordinal integer for a given string, or vice versa.
   */
  function getOrdinal($datasource, $attribute_name, $value, $reset = FALSE) {
    $datasource_id = $datasource->id;

    if (!isset($this->ordinals[$datasource_id]) || $reset) {
      $cid = "searchlight_solr_ordinals_{$datasource_id}";
      if (!$reset && $cache = cache_get($cid)) {
        $this->ordinals[$datasource_id] = $cache->data;
      }
      if ($reset || !isset($this->ordinals[$datasource_id])) {
        $mapping = $this->getOrdinals($datasource);
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
      $this->includeSolr();
      $solr = new solrClient();
      $solr->SetLimits(0, 1000);

      foreach ($attributes as $name => $field) {
        // Dispatch a solr query to retrieve the ordinal.
        $solr->SetGroupBy($name, SPH_GROUPBY_ATTR, '@count desc');
        $solr->SetGroupDistinct($name);
        $result = $solr->Query(NULL, $datasource['id']);

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
   * Convert a solr datestring into a timestamp. Requires a datestring
   * (e.g. 201010) and a target granularity: day, month, year. We reverse-apply
   * Drupal's timezone offset so that this date can be used with format_date(),
   * etc. cleanly.
   */
  function getTimestamp($datestring, $granularity) {
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
}

