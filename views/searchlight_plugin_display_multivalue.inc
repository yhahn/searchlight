<?php

class searchlight_plugin_display_multivalue extends views_plugin_display {
  /**
   * Return the name of this plugin that will be used as the storage key
   * for settings, other identifiers.
   */
  protected function my_name() {
    return 'searchlight_multivalue';
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
  protected function get_datatype($handler, $table, $field) {
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
      $class = get_class($handler);

      // Get the datasource attribute type.
      // We use the handler class for special cases like timestamp where DB
      // column type is not enough information to determine the usage of the
      // field.
      $map = array(
        'serial'  => 'int',
        'int'     => 'int',
        'varchar' => 'text',
        'text'    => 'text',
        'float'   => 'float',
      );
      if (isset($map[$schema['fields'][$field]['type']])) {
        $column_type = $map[$schema['fields'][$field]['type']];
        if ($column_type === 'int' && strpos($class, 'date') !== FALSE) {
          return 'timestamp';
        }
        return isset($map[$column_type]) ? $map[$column_type] : FALSE;
      }
    }
    return FALSE;
  }

  /**
   * Retrieve all fields from field handlers including additional_fields.
   * Requires that the handler belong to a built view.
   */
  protected function get_fields() {
    $clone = $this->view->clone_view();
    $clone->set_display($this->view->current_display);
    $clone->build();
    $handlers = $clone->display_handler->get_handlers('field');

    $fields = array();
    foreach ($handlers as $handler) {
      if ($handler->real_field !== $handler->view->base_field) {
        $fields[$handler->field_alias] = array(
          'label' => $handler->ui_name() . " ({$handler->real_field})",
          'datatype' => $this->get_datatype($handler, $handler->table, $handler->real_field),
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
          if ($field !== $handler->view->base_field || $table !== $handler->view->base_table) {
            $fields[$handler->aliases[$field]] = array(
              'label' => $handler->ui_name() . " ({$field})",
              'datatype' => $this->get_datatype($handler, $table, $field),
              'table' => $table,
              'field' => $field,
              'name' => $handler->aliases[$field],
            );
          }
        }
      }
    }
    return $fields;
  }

  /**
   * Do not fully execute the view -- we only need the build the query object.
   * Generate an array representing a full description of this multivalue.
   */
  function execute() {
    $multivalue = array(
      'label_field' => array(),
      'base_table' => $this->view->base_table,
      'view' => "{$this->view->name}:{$this->view->current_display}",
    );
    $options = $this->get_option($this->my_name());
    $fields = $this->get_fields();
    foreach ($fields as $name => $info) {
      if ($options['field'] === $name) {
        $info['usage'] = 'multivalue';
        $multivalue = array_merge($multivalue, $info);
      }
      else if ($options['label_field'] === $name) {
        $info['usage'] = 'multivalue';
        $multivalue['label_field'] = $info;
      }
    }

    // Override explicitly specified properties if available.
    if (!empty($options['override'])) {
      foreach ($options['override'] as $key => $value) {
        if (!empty($value)) {
          $multivalue[$key] = $value;
        }
      }
    }
    return $multivalue;
  }

  /**
   * Provide the default form for setting options.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    switch ($form_state['section']) {
      case $this->my_name():
        $subform = array('#tree' => TRUE);
        $options = $this->get_option($this->my_name());
        $fields = $this->get_fields();
        foreach ($fields as $name => $info) {
          $field_options[$name] = $info['label'];
        }
        $subform['field'] = array(
          '#type' => 'select',
          '#title' => t('Attribute field'),
          '#description' => t('The selected field will be provided as a multivalue attribute for use with any @basetable datasources.', array('@basetable' => $this->view->base_table)),
          '#options' => $field_options,
          '#default_value' => isset($options['field']) ? $options['field'] : NULL,
        );
        $subform['label_field'] = array(
          '#type' => 'select',
          '#title' => t('Label display'),
          '#description' => t('The selected field will be used to display labels for this attribute when used as facet.'),
          '#options' => $field_options,
          '#default_value' => isset($options['label_field']) ? $options['label_field'] : NULL,
        );
        $form[$this->my_name()] = $subform;
        break;
    }
  }

  /**
   * Provide the summary for attachment options in the views UI.
   *
   * This output is returned as an array.
   */
  function options_summary(&$categories, &$options) {
    parent::options_summary($categories, $options);
    $categories['basic'] = array('title' => t('Searchlight multivalue'));
    $options['searchlight_multivalue'] = array(
      'category' => 'basic',
      'title' => t('Searchlight multivalue'),
      'value' => t('Settings'),
    );
  }
}
