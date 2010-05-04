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
    $base_field = $this->solrGetBaseField($datasource);

    try { 
      $response = $client->search($query, 0, 10, array());
      foreach ($response->response->docs as $doc) {
        $ids[] = $doc->{$base_field};
      }
    }
    catch (Exception $e) {
      drupal_set_message('Caught exception: '.  $e->getMessage(), 'error');
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
  function facetBuild(&$client, $datasource, $query = '', $facets) {
    $options = array('facet' => 'true');

    $options['facet.field'] = array();
    foreach ($facets as $facet) {
      $options['facet.field'][] = $facet['field'];
      $options["f.{$facet['field']}.facet.limit"] = $facet['limit'];
    }

    try {
      // TODO optimize the length of this request.
      $response = $client->search($query, 0, 10, $options);
    }
    catch (Exception $e) {
      drupal_set_message('Caught exception: '. $e->getMessage(), 'error');
      return;
    }

    $built = array();
    foreach ($response->facet_counts->facet_fields as $k => $v) {
      $built[$k] = array();
      foreach ($v as $i => $j) {
        $built[$k][] = array('id' => $i, 'count' => $j);
      }
    }
 
    // TODO
    // foreach ($response->facet_counts->facet_dates) {}

    return $built;
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
}
