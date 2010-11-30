<?php

class SearchlightDatasource {
  var $id;
  var $name;
  var $base_table;
  var $base_field;
  var $fields;
  var $relations;
  var $filters;
  var $options;

  var $view;

  function construct() {
    $this->name = isset($this->name) ? $this->name : '';
    $this->base_table = isset($this->base_table) ? $this->base_table : '';

    $this->base_field = '';
    if ($views_data = views_fetch_data($this->base_table)) {
      $this->base_field = $views_data['table']['base']['field'];
    }

    $this->fields = !empty($this->fields) ? $this->fields : array();
    $this->filters = !empty($this->filters) ? $this->filters : array();
    $this->relations = !empty($this->relations) ? $this->relations : array();
    $this->options = !empty($this->options) ? $this->options : array();
    $this->options = $this->options + array(
      'node_access' => TRUE,
    );

    $this->setId();
  }

  function setId() {
    global $databases;
    $this->id = $databases['default']['default']['database'] . '_' . $this->name;
    return $this;
  }

  function init() {
    return empty($this->view) ? $this->viewInit()->viewSetHandlers() : $this;
  }

  function viewInit() {
    views_include('view');
    $this->view = new view;
    $this->view->base_table = $this->base_table;
    $this->view->api_version = '3.0-alpha1';
    $this->view->new_display('default', 'Default', 'default');
    $this->view->set_display('default');
    return $this;
  }

  function viewSetHandlers() {
    $handler = $this->view->display_handler;
    $handler->override_option('arguments', array());
    $handler->override_option('filters', array());
    $handler->override_option('relationships', array());
    $handler->override_option('sorts', array());

    $fields = array();

    // Force the base field onto the view if possible.
    if ($this->base_field) {
      $fields[$this->base_field] = array(
        'id' => $this->base_field,
        'table' => $this->base_table,
        'field' => $this->base_field,
        'relationship' => 'none',
      );
    }
    foreach ($this->getFields() as $name => $field) {
      $fields[$name] = array(
        'id' => $field['name'],
        'table' => $field['table'],
        'field' => $field['field'],
        'relationship' => $field['relationship'],
      );
    }
    $handler->override_option('fields', $fields);

    $handler->override_option('relationships', $this->getRelations());

    return $this;
  }

  /**
   * Given a Views handler return one of the datasource datatypes based on the
   * database column type and other metadata. May be one of the following
   * datasource attribute types:
   *
   *   `text`:      varchar or text column.
   *   `int`:       serial or int column.
   *   `float`:     float column.
   *   `timestamp`: unix timestamp.
   */
  function getDatatype($handler = NULL, $table, $field) {
    $attribute = array();

    // Ensure that pseudo table names are traced back to their literal tables
    // that actually have a schema definition.
    $data = views_fetch_data();
    if (isset($data[$table]['table']['join'])) {
      foreach ($data[$table]['table']['join'] as $join_info) {
        if (isset($join_info['table']) && $join_info['table'] !== $table) {
          $table = $join_info['table'];
          break;
        }
      }
    }

    $schema = drupal_get_schema($table);
    if ($schema && isset($field, $schema['fields'][$field])) {
      $class = $handler ? get_class($handler) : NULL;
      // Use handler defined in $data in case $data has been altered by another
      // and requires for example another handler
      if (isset($data[$table][$field]['field']['handler'])) {
        $class = $data[$table][$field]['field']['handler'];
      }
      // Get the datasource attribute type.
      // We use the handler class for special cases like timestamp where DB
      // column type is not enough information to determine the usage of the
      // field.
      $map = array(
        'serial' => 'int',
        'int' => 'int',
        'varchar' => 'text',
        'text' => 'text',
        'float' => 'float',
        'numeric' => 'float',
        'decimal' => 'float',
      );
      if (isset($map[$schema['fields'][$field]['type']])) {
        $column_type = $map[$schema['fields'][$field]['type']];
        if ($column_type === 'int' && strpos($class, 'date') !== FALSE) {
          return 'timestamp';
        }
        // Handle date fields with type varchar(map->text) and certain date-time
        // format-types and also custom view handlers containing 'date' in the name.
        if ($column_type === 'text' && (isset($data[$table][$field]['settings']['format_type']) || strpos($class, 'date') !== FALSE)) {
          return 'date';
        }
        return isset($map[$column_type]) ? $map[$column_type] : FALSE;
      }
    }
    // Handle a list of known function/calculation fields.
    $map = array(
      'node_comment_statistics.last_updated' => 'timestamp',
    );
    return isset($map["{$table}.{$field}"]) ? $map["{$table}.{$field}"] : FALSE;
  }

  /**
   * Retrieve fields currently enabled for this datasource.
   */
  function getFields() {
    $fields = array();
    foreach ($this->fields as $name => $field) {
      if ($field['usage'] != 'multivalue') {
        $fields[$name] = $field;
      }
    }
    return $fields;
  }

  /**
   * Retrieve multivalue fields currently enabled for this datasource.
   */
  function getMultivalues() {
    $fields = array();
    foreach ($this->fields as $name => $field) {
      if ($field['usage'] == 'multivalue') {
        $fields[$name] = $field;
      }
    }
    return $fields;
  }

  /**
   * Retrieve relationships currently enabled for this datasource.
   */
  function getRelations() {
    $relationships = array();
    foreach ($this->relations as $name => $relation) {
      $relationships[$name] = $relation;
    }
    return $relationships;
  }

  /**
   * Retrieve all possible multivalue fields for this base table.
   */
  function buildMultivalues() {
    views_invalidate_cache();
    $multivalues = array();

    // Gather all views that have the multivalue display plugin.
    $views = views_get_all_views();
    $usable = array();
    foreach ($views as $view) {
      if ($view->base_table === $this->base_table) {
        foreach ($view->display as $display) {
          if ($display->display_plugin === 'searchlight_multivalue') {
            $usable[$view->name] = $view;
          }
        }
      }
    }
    foreach ($usable as $view) {
      if ($view->name !== 'searchlight_node_access') {
        foreach ($view->display as $display) {
          if ($display->display_plugin === 'searchlight_multivalue') {
            $view = views_get_view($view->name);
            $view->set_display($display->id);
            $multivalue = $view->execute_display();
            $multivalues[$multivalue['name']] = $multivalue;
            $view->destroy();
          }
        }
      }
    }
    return $multivalues;
  }

  /**
   * Build and retrieve node_access multivalue information.
   */
  function buildNodeaccess() {
    views_invalidate_cache();
    $nodeaccess = array();
    if ($this->options['node_access'] && $this->base_table === 'node' && $view = views_get_view('searchlight_node_access')) {
      foreach ($view->display as $display) {
        if ($display->display_plugin === 'searchlight_multivalue') {
          $view = views_get_view($view->name);
          $view->set_display($display->id);
          $multivalue = $view->execute_display();
          $nodeaccess[$multivalue['name']] = $multivalue;
          $view->destroy();
        }
      }
    }
    return $nodeaccess;
  }


  /**
   * Finds all possible relations ships for a field.
   *
   * Orignal code resides in admin.inc of views.
   */
  function getFieldRelationshipOptions($field_id, $field_info) {
    $relationship_options = array();

    $view = &$this->view;
    $item = $view->get_item('default', 'field', $field_id);

    // A whole bunch of code to figure out what relationships are valid for
    // this item.
    $relationships = $view->display_handler->get_option('relationships');
    $relationship_options = array();

    foreach ($relationships as $relationship) {
      $relationship_handler = views_get_handler($relationship['table'], $relationship['field'], 'relationship');
      // ignore invalid/broken relationships.
      if (empty($relationship_handler)) {
        continue;
      }

      // If this relationship is valid for this type, add it to the list.
      $data = views_fetch_data($relationship['table']);
      $base = $data[$relationship['field']]['relationship']['base'];
      $base_fields = views_fetch_fields($base, 'field', $view->display_handler->use_group_by());
      if (isset($base_fields[$item['table'] . '.' . $item['field']])) {
        $relationship_handler->init($view, $relationship);
        $relationship_options[$relationship['id']] = $relationship_handler->label();
      }
    }

    if (!empty($relationship_options)) {
      // Make sure the existing relationship is even valid. If not, force
      // it to none.
      $base_fields = views_fetch_fields($view->base_table, 'field', $view->display_handler->use_group_by());
      if (isset($base_fields[$item['table'] . '.' . $item['field']])) {
        $relationship_options = array_merge(array('none' => t('Do not use a relationship')), $relationship_options);
      }
    }
    return $relationship_options;
  }

  /**
   * Provide the default form for setting options.
   */
  function optionsForm(&$form, &$form_state) {
    views_include('admin');
    views_include('form');

    $this->viewInit()->viewSetHandlers();
    $base_tables = $this->view->get_base_tables();

    // Theme function will handle formatting of datasource information.
    $form['#theme'] = 'searchlight_admin_datasource';
    $form['#datasource'] = $this;

    // Calculations of indexing percentage.
    $result = db_query("SELECT count(id) AS count, status FROM {searchlight_search} WHERE type = :type GROUP BY status", array(':type' => $this->base_table));
    $items_indexed = 0;
    $items_total = 0;
    foreach ($result as $row) {
      $items_indexed = $row->status == 1 ? $row->count : $items_indexed;
      $items_total = $items_total + $row->count;
    }
    $form['index']['#tree'] = TRUE;
    $form['index']['percent'] = array(
      '#markup' => !empty($items_total) ? number_format(($items_indexed / $items_total * 100), 1) . '%' : '0%',
    );
    $form['index']['counts'] = array(
      '#markup' => t('@indexed of @total total', array(
        '@indexed' => $items_indexed,
        '@total' => $items_total,
      )),
    );
    $form['index']['invalidate'] = array(
      '#type' => 'submit',
      '#value' => t('Queue for reindexing'),
    );

    $form['help'] = array(
      '#type' => 'item',
      '#markup' => t('Choose a usage type for each field in the search datasource. <strong>Content</strong> fields will be used to perform text searches. <strong>Attributes</strong> can be used to filter, sort or group the search results.'),
    );

    $form['fields'] = array(
      '#prefix' => "<div id='datasource-fields'>",
      '#suffix' => "</div>",
      '#tree' => TRUE,
      '#theme' => 'searchlight_admin_datasource_fields',
      '#title' => t('Fields'),
    );

    // Add fields
    $field_options = array();
    $fields = views_fetch_fields(array_keys($base_tables), 'field', TRUE);

    foreach ($fields as $field_id => $info) {
      $field_options[$info['group']][$field_id] = $info['title'];
    }
    $form['fields']['new']['#tree'] = TRUE;
    $form['fields']['new']['field'] = array(
      '#type' => 'select',
      '#options' => $field_options,
    );
    $form['fields']['new']['add'] = array(
      '#value' => t('Add field'),
      '#type' => 'submit',
      '#submit' => array('searchlight_admin_datasource_edit_submit'),
      '#limit_validation_errors' => array(array('fields', 'new')),
      // @TODO: Determine why AJAX is failing. Possibly because the element to
      // which the ajax behavior is attached is replaced by the ajax callback.
      // '#ajax' => array(
      //   'callback' => 'searchlight_admin_datasource_ajax_fields_add',
      //   'wrapper' => 'datasource-fields',
      //   'method' => 'replace',
      // ),
    );

    // Remove fields
    if (count($this->getFields())) {
      $form['fields']['remove'] = array(
        '#value' => t('Remove selected fields'),
        '#type' => 'submit',
        '#submit' => array('searchlight_admin_datasource_edit_submit'),
        '#limit_validation_errors' => array(array('fields', 'fields')),
        // '#ajax' => array(
        //   'callback' => 'searchlight_admin_datasource_ajax_fields_remove',
        //   'wrapper' => 'datasource-fields',
        //   'method' => 'replace',
        // ),
      );
    }
    // Adjust existing fields
    $form['fields']['fields'] = array('#tree' => TRUE);
    foreach ($this->getFields() as $name => $info) {
      $form['fields']['fields'][$name] = array();

      // Remove field checkbox.
      $form['fields']['fields'][$name]['remove'] = array(
        '#type' => 'checkbox',
        '#default_value' => FALSE,
      );

      // Relation
      if (count($relation_options = $this->getFieldRelationshipOptions($name, $info)) > 1) {

        $form['fields']['fields'][$name]['relationship'] = array(
          '#type' => 'select',
          '#options' => $relation_options,
          '#default_value' => ((!empty($info['relationship'])) ? $info['relationship'] : NULL),
        );
      }
      else {
        $relation = reset($relation_options);
        $form['fields']['fields'][$name]['relation_name'] = array(
          '#markup' => ((empty($relation)) ? t('none') : $relation),
        );
        $form['fields']['fields'][$name]['relationship'] = array(
          '#type' => 'hidden',
          '#value' => ((empty($relation)) ? t('none') : $relation),
        );
      }

      // Field label.
      $form['fields']['fields'][$name]['label'] = array(
        '#markup' => $info['label'],
      );

      // Datatype
      $form['fields']['fields'][$name]['datatype'] = array(
        '#markup' => "<code>{$info['datatype']}</code>",
      );

      // Usage
      $default_usage = ($info['datatype'] === 'text') ? 'content' : 'attribute';
      $default_usage = isset($this->fields[$name]['usage']) ? $this->fields[$name]['usage'] : $default_usage;
      $form['fields']['fields'][$name]['usage'] = array(
        '#type' => 'select',
        '#options' => array(
          'content' => t('Content'),
          'attribute' => t('Attribute'),
        ),
        '#default_value' => $default_usage,
      );
    }

    // Handle multivalues.
    // Add fields
    $multivalue_options = array();
    foreach ($this->buildMultivalues() as $name => $info) {
      if (!array_key_exists($name, $this->getMultivalues())) {
        $multivalue_options[$name] = $info['label'];
      }
    }
    $form['multivalues'] = array(
      '#tree' => TRUE,
      '#theme' => 'searchlight_admin_datasource_fields',
      '#title' => t('Multivalues'),
      '#prefix' => "<div id='datasource-multivalues'>",
      '#suffix' => "</div>",
    );
    $form['multivalues']['new']['#tree'] = TRUE;
    $form['multivalues']['new']['field'] = array(
      '#type' => 'select',
      '#options' => $multivalue_options,
    );
    $form['multivalues']['new']['add'] = array(
      '#value' => t('Add multivalue'),
      '#type' => 'submit',
      '#submit' => array('searchlight_admin_datasource_edit_submit'),
      '#limit_validation_errors' => array(array('multivalues', 'new')),
    // '#ajax' => array(
    //   'callback' => 'searchlight_admin_datasource_ajax_multivalues_add',
    //   'wrapper' => 'datasource-multivalues',
    //   'method' => 'replace',
    // ),
    );

    // Remove fields
    if (count($this->getMultivalues())) {
      $form['multivalues']['remove'] = array(
        '#value' => t('Remove selected multivalues'),
        '#type' => 'submit',
        '#submit' => array('searchlight_admin_datasource_edit_submit'),
        '#limit_validation_errors' => array(array('multivalues', 'fields')),
      // '#ajax' => array(
      //   'callback' => 'searchlight_admin_datasource_ajax_multivalues_remove',
      //   'wrapper' => 'datasource-multivalues',
      //   'method' => 'replace',
      // ),
      );
    }
    $form['multivalues']['fields'] = array();
    foreach ($this->getMultivalues() as $name => $info) {
      $form['multivalues']['fields'][$name] = array();

      // Remove field checkbox.
      $form['multivalues']['fields'][$name]['remove'] = array(
        '#type' => 'checkbox',
        '#default_value' => FALSE,
      );

      // Field label.
      $form['multivalues']['fields'][$name]['label'] = array(
        '#markup' => $info['label'],
      );

      // Datatype
      $form['multivalues']['fields'][$name]['datatype'] = array(
        '#markup' => "<code>{$info['datatype']}</code>",
      );

      $form['multivalues']['fields'][$name]['usage'] = array(
        '#markup' => t('Multivalue'),
      );
    }

    $form['relations'] = array(
      '#prefix' => "<div id='datasource-relations'>",
      '#suffix' => "</div>",
      '#tree' => TRUE,
      '#theme' => 'searchlight_admin_datasource_relations',
      '#title' => t('Relations'),
    );

    // Add relations
    $relation_options = array();
    $fields = views_fetch_fields(array_keys($base_tables), 'relationship');
    foreach ($fields as $relation_id => $info) {
      $relation_options[$info['group']][$relation_id] = $info['title'];
    }
    $form['relations']['new']['#tree'] = TRUE;
    $form['relations']['new']['relation'] = array(
      '#type' => 'select',
      '#options' => $relation_options,
    );
    $form['relations']['new']['add'] = array(
      '#value' => t('Add relation'),
      '#type' => 'submit',
      '#submit' => array('searchlight_admin_datasource_edit_submit'),
      '#limit_validation_errors' => array(array('relations', 'new')),
    );

    $relations = $this->getRelations();
    // Remove relations
    if (count($relations)) {
      $form['relations']['remove'] = array(
        '#value' => t('Remove selected relations'),
        '#type' => 'submit',
        '#submit' => array('searchlight_admin_datasource_edit_submit'),
        '#limit_validation_errors' => array(array('relations', 'relations')),
      );
    }
    // Adjust existing relations
    $form['relations']['relations'] = array('#tree' => TRUE);
    foreach ($relations as $name => $info) {
      $form['relations']['relations'][$name] = array();

      // Remove field checkbox.
      $form['relations']['relations'][$name]['remove'] = array(
        '#type' => 'checkbox',
        '#default_value' => FALSE,
      );

      // Relation label.
      $form['relations']['relations'][$name]['label'] = array(
        '#type' => 'textfield',
        '#default_value' => $info['label'],
        '#field_suffix' => '(' . $info['table'] . '.' . $info['field'] . ')',
      );

      // Required relation.
      $form['relations']['relations'][$name]['required'] = array(
        '#type' => 'checkbox',
        '#default_value' => (int) $info['required'],
      );
    }

    $backend = variable_get('searchlight_backend');
    $settings = variable_get('searchlight_backend_' . $backend);
    if ($settings['grouping']) {
      // Group
      $form['groupfield'] = array(
        '#tree' => TRUE,
        '#theme' => 'searchlight_admin_datasource_fields',
        '#title' => t('Group'),
        '#prefix' => "<div id='datasource-fields'>",
        '#suffix' => "</div>",
      );

      // Add fields
      $groupfield_options = array();
      foreach ($this->getFields() as $name => $info) {
        $groupfield_options[$name] = $info['label'];
      }
      $form['groupfield']['new']['#tree'] = TRUE;
      $form['groupfield']['new']['field'] = array(
        '#type' => 'select',
        '#options' => $groupfield_options,
      );
      $form['groupfield']['new']['add'] = array(
        '#value' => t('Add group field'),
        '#type' => 'submit',
        '#submit' => array('searchlight_admin_datasource_edit_submit'),
        '#limit_validation_errors' => array(array('groupfield', 'new')),
      );

      // Remove field
      if (isset($this->options['groupfield'])) {
        $form['groupfield']['remove'] = array(
          '#value' => t('Remove groupfield'),
          '#type' => 'submit',
          '#submit' => array('searchlight_admin_datasource_edit_submit'),
          '#limit_validation_errors' => array(array('groupfield', 'fields')),
        );
      }
      $form['groupfield']['fields'] = array();
      if (isset($this->options['groupfield'])) {
        $form['groupfield']['fields'][$this->options['groupfield']] = array();

        // Remove field checkbox.
        $form['groupfield']['fields'][$this->options['groupfield']]['remove'] = array(
                '#type' => 'checkbox',
                '#default_value' => FALSE,
        );

        // Field label.
        $form['groupfield']['fields'][$this->options['groupfield']]['label'] = array(
          '#markup' => $this->fields[$this->options['groupfield']]['label'],
        );

        // Datatype
        $form['groupfield']['fields'][$this->options['groupfield']]['datatype'] = array(
                '#markup' => "<code>{$this->fields[$this->options['groupfield']]['datatype']}</code>",
        );

        $form['groupfield']['fields'][$this->options['groupfield']]['usage'] = array(
                '#markup' => t('Grouping'),
        );
      }
    }

    return $form;
  }

  /**
   * Perform any necessary changes to the form values prior to storage.
   * There is no need for this function to actually store the data.
   */
  function optionsSubmit($form, &$form_state) {
    switch ($form_state['clicked_button']['#id']) {
      case 'edit-index-invalidate':
        searchlight_datasource_save($this);
        drupal_set_message(t('The index for this datasource needs to be rebuilt.'));
        break;
      case 'edit-fields-new-add':
        list($table, $field) = explode('.', $form_state['values']['fields']['new']['field']);
        $this->addField($table, $field);
        searchlight_datasource_save($this, TRUE);
        break;
      case 'edit-fields-remove':
        foreach ($form_state['values']['fields']['fields'] as $name => $values) {
          if (!empty($values['remove'])) {
            $this->removeField(NULL, NULL, $name);
          }
        }
        searchlight_datasource_save($this, TRUE);
        break;
      case 'edit-multivalues-new-add':
        $this->addMultivalue($form_state['values']['multivalues']['new']['field']);
        searchlight_datasource_save($this, TRUE);
        break;
      case 'edit-multivalues-remove':
        foreach ($form_state['values']['multivalues']['fields'] as $name => $values) {
          if (!empty($values['remove'])) {
            $this->removeMultivalue($name);
          }
        }
        searchlight_datasource_save($this, TRUE);
        break;
      case 'edit-groupfield-new-add':
        $this->options['groupfield'] = $form_state['values']['groupfield']['new']['field'];
        searchlight_datasource_save($this, TRUE);
        break;
      case 'edit-groupfield-remove':
        foreach ($form_state['values']['groupfield']['fields'] as $name => $values) {
          if (!empty($values['remove'])) {
            unset ($this->options['groupfield']);
          }
        }
        searchlight_datasource_save($this, TRUE);
        break;
      case 'edit-relations-new-add':
        list($table, $field) = explode('.', $form_state['values']['relations']['new']['relation']);
        $this->addRelation($table, $field);
        searchlight_datasource_save($this, TRUE);
        break;
      case 'edit-relations-remove':
        foreach ($form_state['values']['relations']['relations'] as $name => $values) {
          if (!empty($values['remove'])) {
            $this->removeRelation($name);
          }
        }
        searchlight_datasource_save($this, TRUE);
        break;
      case 'edit-save':
        // Save additional metadata from fields, multivalues.
        foreach (array('fields', 'multivalues') as $key) {
          if (!empty($form_state['values'][$key]['fields'])) {
            foreach ($form_state['values'][$key]['fields'] as $name => $values) {
              $values = array_diff_key($values, array('remove' => NULL));
              if (isset($this->{$key}[$name])) {
                $this->{$key}[$name] = array_merge($this->{$key}[$name], $values);
              }
            }
          }
        }
        // Save options.
        $this->options = isset($form_state['values']['options']) ? $form_state['values']['options'] : $this->options;

        // Save relations
        if (!empty($form_state['values']['relations']['relations'])) {
          foreach ($form_state['values']['relations']['relations'] as $name => $values) {
            $values = array_diff_key($values, array('remove' => NULL));
            if (isset($this->relations[$name])) {
              $this->relations[$name] = array_merge($this->relations[$name], $values);
            }
          }
        }

        // Save the datasource.
        searchlight_datasource_save($this);
        drupal_set_message(t('Datasource @datasource saved. The index for this datasource needs to be rebuilt.', array('@datasource' => $this->name)));
        break;

      case 'edit-relations-new-add':
        $this->addRelation($form_state['values']['relations']['new']['table'], $form_state['values']['relations']['new']['field']);
        searchlight_datasource_save($this, TRUE);
        break;
      case 'edit-relations-remove':
        foreach ($form_state['values']['relations']['relations'] as $name => $values) {
          if (!empty($values['remove'])) {
            $this->removeRelation(NULL, NULL, $name);
          }
        }
        searchlight_datasource_save($this, TRUE);
        break;
    }
  }


  /**
   * Remove relation.
   * @param string $relation
   */
  function removeRelation($name) {
    if (array_key_exists($name, $this->relations)) {
      unset($this->relations[$name]);
    }
    return $this->viewInit()->viewSetHandlers();
  }

  /**
   * Adds a new relation for the datasource.
   * @param string $table
   * @param string $field
   * @param boolean $required
   */
  function addRelation($table, $field, $label = NULL, $required = FALSE) {
    $this->viewInit()->viewSetHandlers();

    if (empty($label)) {
      $label = $table . '.' . $field;
    }

    $options = array(
      'label' => $label,
      'required' => (bool) $required,
    );
    $id = $this->view->add_item('default', 'relationship', $table, $field, $options);

    $this->relations = $this->view->display_handler->get_option('relationships');
  }

  function addField($table, $field) {
    // Add the field to the view and build. This will give us an inited handler
    // with full aliases.
    $this->viewInit()->viewSetHandlers();
    $fields = $this->view->display_handler->get_option('fields');
    $fields[$field] = array(
      'id' => $field,
      'table' => $table,
      'field' => $field,
      'relationship' => 'none',
    );

    $this->view->display_handler->set_option('fields', $fields);
    $this->view->build();

    // Retrieve field information for storage with datasource.
    $handler = isset($this->view->field[$field]) ? $this->view->field[$field] : NULL;

    // Track fields added so we can provide a default usage based on datatype.
    $fields_added = array();

    // Don't use alias for base field.
    if ($field === $this->view->base_field) {
      $fields_added[] = $table . '.' . $field;

      $this->fields[$field] = array(
        'label' => $field,
        'datatype' => $this->getDatatype(NULL, $table, $field),
        'table' => $table,
        'field' => $field,
        'name' => $field,
      );
    }
    // Use alias for all other fields.
    if ($handler) {
      $fields_added[] = $handler->table . '.' . $handler->field_alias;

      $this->fields[$handler->table . '.' . $handler->field_alias] = array(
        'label' => $handler->ui_name() . " ({$handler->real_field})",
        'datatype' => $this->getDatatype($handler, $handler->table, $handler->real_field),
        'table' => $handler->table,
        'field' => $handler->real_field,
        'name' => $handler->field_alias,
      );
    }
    if (!empty($handler->additional_fields)) {
      foreach ($handler->additional_fields as $identifier => $info) {
        $table = $handler->table;
        $field = $info;
        // If the additional fields have specified table/field in an array,
        // use these.
        if (is_array($info) && isset($info['field'])) {
          $table = isset($info['table']) ? $info['table'] : $table;
          $field = $info['field'];
        }

        // Check if we are dealing with a field API field handler.
        // - Skip the actual field specified by the handler (entity_id).
        // - Exclude field metadata in additional_fields (language, etid, delta, etc.)
        $add_field = FALSE;
        if (isset($handler->definition['field_info'], $handler->definition['entity_tables'])) {
          $add_field = !in_array($field, array('delta', 'etid', 'language')) && strpos($field, '_format') === FALSE;
        }
        else {
          $add_field = $field !== $handler->view->base_field || $table !== $handler->view->base_table;
        }
        if ($add_field) {
          $fields_added[] = $table . '.' . $handler->aliases[$field];

          $this->fields[$table . '.' . $handler->aliases[$field]] = array(
            'label' => $handler->ui_name() . " ({$field})",
            'datatype' => $this->getDatatype($handler, $table, $field),
            'table' => $table,
            'field' => $field,
            'name' => $handler->aliases[$field],
          );
        }
      }
    }

    if (sizeof($fields_added)) {
      foreach ($fields_added as $name) {
        if (!isset($this->fields[$name]['usage'])) {
          $this->fields[$name]['usage'] = ($this->fields[$name]['datatype'] === 'text') ? 'content' : 'attribute';
        }
      }
    }
    return $this->viewInit()->viewSetHandlers();
  }

  function removeField($table = NULL, $field = NULL, $name = NULL) {
    if (isset($name)) {
      if (array_key_exists($name, $this->fields)) {
        unset($this->fields[$name]);
      }
    }
    elseif (isset($table, $field)) {
      foreach ($this->fields as $name => $field) {
        if ($field['table'] === $table && $field['field'] === $field) {
          unset($this->fields[$name]);
        }
      }
    }
    return $this->viewInit()->viewSetHandlers();
  }

  function addMultivalue($name) {
    $multivalues = $this->buildMultivalues();
    if (isset($multivalues[$name])) {
      $this->fields[$name] = $multivalues[$name];
    }
    return $this;
  }

  function removeMultivalue($name) {
    return $this->removeField(NULL, NULL, $name);
  }
}
