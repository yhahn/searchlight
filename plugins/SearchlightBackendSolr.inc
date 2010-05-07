<?php

/**
 * Provides integration with Apache Solr.
 */
class SearchlightBackendSolr extends SearchlightBackend {
  /**
   * Override of settingsDefault().
   */
  function settingsDefault() {
    return array(
      'host' => 'localhost',
      'port' => 8983,
      'path' => '/solr',
      'jar_path' => '/usr/local/apache-solr-1.4.0/example',
      'item_limit' => 100,
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
    $form['item_limit'] = array(
      '#title' => t('Items to index per job'),
      '#type' => 'select',
      '#default_value' => $this->settings['item_limit'],
      '#options' => array(
        0 => t('Unlimited'),
        100 => 100,
        250 => 250,
        500 => 500,
        750 => 750,
        1000 => 1000,
      ),
    );
    return $form;
  }

  /**
   * Override of initClient().
   */
  function initClient($datasource) {
    $this->solrInclude();
    $path = $this->settings['path'] .'/'. $datasource->id;
    $client = new Apache_Solr_Service($this->settings['host'], (int) $this->settings['port'], $path);
    return $client;
  }

  /**
   * Override of executeQuery().
   */
  function executeQuery(&$client, $datasource, $query = '') {
    $query = $this->solrPrepareQuery($client, $datasource, $query);
    $offset = $client->searchlightOffset;
    $limit = $client->searchlightLimit;

    $options = array();
    if (isset($client->searchlightSort)) {
      foreach ($client->searchlightSort as $field => $order) {
        $field = ($field == 'searchlight_weight' ? 'score' : $field);
        $sort .= $field .' '. strtolower($order) .',';
      }
      $options['sort'] = rtrim($sort, ',');
    }

    try { 
      $response = $client->search($query, $offset, $limit, $options);
    }
    catch (Exception $e) {
      drupal_set_message('Caught exception: '.  $e->getMessage(), 'error');
      return FALSE;
    }

    foreach ($response->response->docs as $doc) {
      $ids[] = $doc->{$datasource->base_field};
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
   * Set a filter parameter for the search backend.
   */
  function setFilter(&$client, $datasource, $filters) {
    if (!empty($filters)) {
      $client->searchlightFilters = array();
      $range_filters = array();
      foreach ($filters as $params) {
        $field    = $params['field'];
        $operator = $params['operator'];
        $args     = $params['args'];

        switch($operator) {
          case '<':
          case '<=':
            $range_filters[$field]['max'] = $args[0];
            break;
          case '>':
          case '>=':
            $range_filters[$field]['min'] = $args[0];
            break;
          case '=':
            $client->searchlightFilters[] = $field .':"'. $args[0] .'"';
            break;
          case '!=':
          case '<>':
            $client->searchlightFilters[] = '-'. $field .':"'. $args[0] .'"';
            break;
          case 'IN':
            foreach ($args as $arg) {
              $client->searchlightFilters[] = "{$field}:$arg";
            }
            break;
          case 'NOT IN':
            foreach ($args as $arg) {
              $client->searchlightFilters[] = "-{$field}:$arg";
            }
            break;
          default:
            //dsm($params);
            break;
        }
      }
      foreach ($range_filters as $field => $range) {
        $min = isset($range['min']) ? $range['min'] : '*';
        $max = isset($range['max']) ? $range['max'] : '*';
        $client->searchlightFilters[] = "{$field}:[ $min TO $max ]";
      }
    }
  }

  /**
   * Set a sort parameter for the search backend.
   */
  function setSort(&$client, $datasource, $sorts) {
    if (!empty($sorts)) {
      $client->searchlightSort = array();
      foreach ($sorts as $sort) {
        $client->searchlightSort[$sort['field']] = $sort['order'];
      }
    }
  }


  /**
   * Override of setPager().
   */
  function setPager(&$client, $offset, $limit) {
    $client->searchlightOffset = (int) $offset;
    $client->searchlightLimit = (int) $limit;
  }

  /**
   * Override of setNodeAccess().
   */
  function setNodeAccess(&$client, $grants) {
    /*
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
    */
  }

  /**
   * Override of dataRange().
   */
  function dateRange($timestamp, $granularity = 'month') {
    $unixtime = strtotime($timestamp);
    $range = parent::dateRange($unixtime, $granularity);
    return array(
      'from' => $timestamp,
      'to' => gmdate('Y-m-d\\TH:i:s\\Z', $range['to']),
    );
  }

  /**
   * Override of facetBuild().
   */
  function facetBuild(&$client, $datasource, $query = '', $facets) {
    $query = $this->solrPrepareQuery($client, $datasource, $query);

    $limit_options = $options;
    $options = array(
      'facet' => 'true',
      'facet.mincount' => '1',
      'facet.field' => array(),
      'facet.date' => array(),
    );

    foreach ($facets as $facet) {
      $options["f.{$facet['field']}.facet.limit"] = $facet['limit'];

      $field = $datasource->fields[$facet['field']];
      switch ($field['datatype']) {
        case 'timestamp':
          // TODO cache this value so it's not requested on every page load.
          $limits = array(
            'max' => $facet['field'] ." desc",
            'min' => $facet['field'] ." asc",
          );

          foreach ($limits as $lim => $sort) {
            $limit_options['sort'] = $sort;
            try {
              // We ask for two items here because, for some unknown reason
              // Solr will return some random, unsorted, element as the last
              // item for certain date sorts. This is a hack.
              $response = $client->search($query, 0, 2, $limit_options);
              $limits[$lim] = $response->response->docs[0]->{$facet['field']};
            }
            catch (Exception $e) {
              drupal_set_message('Caught exception: '. $e->getMessage(), 'error');
              return;
            }
          }

          $options['facet.date'][] = $facet['field'];
          // @TODO several issues here
          //      - start/end will need to be adjusted to match an even started
          //        point for the gap so that dates all fit within the declared 
          //        gap, and we avoid "midnight" problems.
          //      - the month/day/time is set to Jan 1 00:00:00 as granularity/gaps
          //        are calculated as increments from the starting point. HOWEVER,
          //        post Solr query, Drupal's format_date() will apply a user/site
          //        timezone adjustment before formatting the date. This means that,
          //        for example, year granularity can give us incorrect time displays
          //        to the user. Proposed solution is to apply an inverse of the TZ
          //        adjustment to the start/end dates *before* sending to Solr.
          $options["f.{$facet['field']}.facet.date.start"] = $limits['min'];
          $options["f.{$facet['field']}.facet.date.end"] = $limits['max'];

          switch ($facet['granularity']) {
            case 'day':
              $gap = '+1DAY';
              break;
            case 'year':
              $gap = '+1YEAR';
              break;
            case 'week':
              $gap = '+1WEEK';
              break;
            case 'month':
            default:
              $gap = '+1MONTH';
              break;
          }
          $options["f.{$facet['field']}.facet.date.gap"] = $gap;
          break;
        default:
          $options['facet.field'][] = $facet['field'];
          break;
      }
    }

    try {
      $response = $client->search($query, 0, 1, $options);
    }
    catch (Exception $e) {
      drupal_set_message('Caught exception: '. $e->getMessage(), 'error');
      return;
    }

    $built = array();
    if (isset($response->facet_counts->facet_fields)) {
      foreach ($response->facet_counts->facet_fields as $k => $v) {
        $built[$k] = array();
        foreach ($v as $i => $j) {
          $built[$k][$i] = array('id' => $i, 'count' => $j);
        }
      }
    }

    foreach ($response->facet_counts->facet_dates as $k => $v) {
      foreach ($v as $i => $j) {
        // Filter out empty ranges.
        if (!empty($j) && !in_array($i, array('gap', 'end'))) {
          // $time = gmmktime($i);
          // $build[$k][$i]format_date($time);
          $built[$k][$i] = array('id' => $i, 'count' => $j);
        }
      }
    }
    return $built;
  }

  /**
   * Override of invalidateIndex().
   */
  function invalidateIndex($datasource) {
    variable_del('searchlight_solr_last');

    $solr = $this->initClient($datasource);
    try {
      $solr->deleteByQuery('[* TO *]');
      $solr->commit();
    }
    catch (Exception $e) {
      drupal_set_message('Caught exception: '. $e->getMessage(), 'error');
      return;
    }
  }

  /**
   * Override of drushSearchd().
   * Start the Solr service.
   */
  function drushSearchd() {
    $file_path = conf_path() .'/solr';
    $solr_home = drush_locate_root() .'/'. conf_path() . '/solr';
    $log_dir = $solr_home .'/log';
    if (file_check_directory($log_dir, TRUE)) {
      $opts .= '-Dsolr.solr.home='. $solr_home .' ';
      $opts .= '-Djetty.logs='. $log_dir.' ';
      $opts .= '-Djetty.home='. $this->settings['jar_path'] .' ';
      $opts .= '-jar '. $this->settings['jar_path'] .'/start.jar';
      drush_op('drush_shell_exec', 'java '. $opts);
    }
    return drush_log("An error ocurred while starting the search daemon.", 'error');
  }

  /**
   * Override of drushIndex().
   * Run an indexing job. Requires that the Solr service is available.
   */
  function drushIndex() {
    // build list of this site's indexes
    $indexes = array();
    foreach (searchlight_get_datasource() as $datasource) {
      // Add the base field to the datasource field representation.
      $datasource->addField($datasource->base_table, $datasource->base_field);

      $datasource->viewInit();
      $datasource->view->new_display('searchlight_solr', 'Searchlight Solr', 'searchlight_solr');
      $datasource->view->set_display('searchlight_solr');
      $datasource->viewSetHandlers();
      $handler = $datasource->view->display_handler;

      // Add sort
      $sorts[$datasource->base_field] = array(
        'id' => $datasource->base_field,
        'table' => $datasource->base_table,
        'field' => $datasource->base_field,
      );
      $handler->override_option('sorts', $sorts);

      $datasource->view->build();

      // Build query object and add the "fresh" join.
      $datasource->view->built = false;
      $this->queryFresh($datasource->view->query);

      // Set limit for number of items to index at once.
      $datasource->view->set_items_per_page($this->settings['item_limit']);

      $items = $datasource->view->render();
      $raw_items = $datasource->view->result;

      if (!empty($items)) {
        $solr = $this->initClient($datasource);
        $documents = array();
        $ids = array();
        foreach($items as $pos => $item) {
          $document = new Apache_Solr_Document();
          $content = array();
          foreach ($datasource->fields as $field) {
            if ($field['usage'] == 'content') {
              $content[] = $this->apachesolr_clean_text($item[$field['name']]);
            }
            else {
              if ($field['datatype'] == 'timestamp') {
                $document->{$field['name']} = gmdate('Y-m-d\\TH:i:s\\Z', $raw_items[$pos]->{$field['name']});
              }
              else {
                $document->{$field['name']} = $raw_items[$pos]->{$field['name']};
              }
            }
          }
          $ids[] = $item[$datasource->view->base_field];
          $document->_body = implode(' ', $content);
          $documents[] = $document;
        }

        $solr->addDocuments($documents);
        $solr->commit();
        // $solr->optimize(); //merges multiple segments into one

        $args = $ids;
        array_unshift($args, $datasource->base_table);
        db_query("UPDATE {searchlight_search} SET status = 1 WHERE type ='%s'
                  AND id IN (". rtrim(str_repeat('%d,', count($ids)), ',') .")", $args);

        drush_log(dt('@datasource: Indexing completed for @count items.', array('@datasource' => $datasource->name, '@count' => count($ids))), 'success');
      }
      else {
        drush_log(dt('@datasource: Nothing to index.', array('@datasource' => $datasource->name)), 'success');
      }

      // Deletion.
      $result = db_query("SELECT * FROM {searchlight_search} WHERE status = -1 AND type = '%s'", $datasource->base_table);
      $delete = array();
      while ($row = db_fetch_object($result)) {
        $delete[] = $row->id;
      }
      if (!empty($delete)) {
        $solr = $this->initClient($datasource);
        foreach ($delete as $id) {
          $solr->deleteById($id);
        }
        $solr->commit();

        // Once updates are complete, remove the Drupal-side records.
        db_query("DELETE FROM {searchlight_search} WHERE status = -1 AND type = '%s'", $datasource->base_table);

        drush_log(dt('@datasource: Deletion completed for @count items.', array('@datasource' => $datasource->name, '@count' => count($delete))), 'success');
      }
      variable_set('searchlight_solr_last', time());
    }
  }

  /**
   * Override of drushConf().
   * Write the Solr configuration files.
   */
  function drushConf() {
    $file_path = conf_path() .'/solr';
    if (file_check_directory($file_path, TRUE)) {
      // Collect configuration arrays for each datasource.
      $cores = array();
      foreach (searchlight_get_datasource() as $datasource) {
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
   * From the apachsolr module.
   */
  function apachesolr_clean_text($text) {
    // Add spaces before stripping tags to avoid running words together.
    $text = filter_xss(str_replace(array('<', '>'), array(' <', '> '), $text), array());
    // Decode entities and then make safe any < or > characters.
    return htmlspecialchars(html_entity_decode($text, ENT_NOQUOTES, 'UTF-8'), ENT_NOQUOTES, 'UTF-8');
  }

  /**
   * Assemble the query string.
   */
  function solrPrepareQuery(&$client, $datasource, $query) {
    $query = empty($query) ? '[* TO *]' : '"'. $query .'"'; //  Is this really right?
    if (isset($client->searchlightFilters)) {
      $query .= ' '. implode(' ', $client->searchlightFilters);
    }
    return $query;
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
    $conf['conf']['unique_key'] = $datasource->base_field;

    // Add the base field to the datasource field representation.
    $datasource->addField($datasource->base_table, $datasource->base_field);

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
  function solrInclude() {
    module_load_include('php', 'searchlight', 'libraries/SolrPhpClient/Apache/Solr/Service');
  }
}
