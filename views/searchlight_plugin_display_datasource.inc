<?php

class searchlight_plugin_display_datasource extends views_plugin_display {
  /**
   * Return the name of this plugin that will be used as the storage key
   * for settings, other identifiers.
   */
  protected function my_name() {
    return 'searchlight_datasource';
  }

  /**
   * Get the final name for a given datasource field. Based on the field alias
   * assigned by Views **after** the View has been built.
   */
  protected function field_name($handler) {
    if ($handler->field_alias != 'unknown') {
      return $handler->field_alias;
    }
    else if (isset($handler->aliases[$handler->real_field])) {
      return $handler->aliases[$handler->real_field];
    }
    return FALSE;
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
   * Retrieve all possible multivalue fields for this base table.
   */
  protected function get_multivalues() {
    $multivalues = array();
    foreach (searchlight_get_views_by_display($this->view->base_table, 'searchlight_multivalue') as $view) {
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
  protected function get_nodeaccess() {
    views_invalidate_cache();
    $nodeaccess = array();
    if ($view = views_get_view('searchlight_node_access')) {
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
   * Static function with key value pairs for options that should have
   * their values forced. UI components for these options will be hidden.
   */
  protected function force_options() {
    return array(
      'style_plugin'   => 'default',
      'style_options'  => NULL,
      'row_plugin'     => 'fields',
      'row_options'    => NULL,
      'access'         => NULL,
      'cache'          => NULL,
      'title'          => NULL,
      'use_ajax'       => NULL,
      'items_per_page' => NULL,
      'user_pager'     => NULL,
      'distinct'       => NULL,
      'sorts'          => array(),
      'arguments'      => array(),
      'header'         => '',
      'footer'         => '',
      'empty'          => '',
      'exposed_block'  => FALSE,
      'analyze-theme'  => NULL,
    );
  }

  /**
   * Just say no.
   */
  function uses_breadcrumb() { return FALSE; }

  /**
   * Show no output.
   */
  function render() {
    parent::render();
    return '';
  }

  /**
   * Do not fully execute the view -- we only need the build the query object.
   * Generate an array representing a full description of this datasource.
   */
  function execute() {
    $options = $this->get_option($this->my_name());
    $this->view->build();
    $datasource = array(
      'fields' => array(),
      'base_table' => $this->view->base_table,
      'id' => searchlight_get_datasource_id($this->view->base_table),
      'view' => "{$this->view->name}:{$this->view->current_display}",
    );

    // Iterate through each handler and generate full field information.
    $datasource['fields'] = $options['fields'];
    $fields = array_merge($this->get_fields(), $this->get_multivalues());
    foreach ($fields as $name => $info) {
      if (isset($datasource['fields'][$name])) {
        $datasource['fields'][$name] += $info;
      }
    }

    // Add in nodeaccess information.
    if ($this->view->base_table === 'node' && !empty($options['node_access'])) {
      $nodeaccess = $this->get_nodeaccess();
      foreach ($nodeaccess as $name => $info) {
        $datasource['fields'][$name] = $info;
      }
    }

    return $datasource;
  }

  /**
   * Provide the default form for setting options.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    switch ($form_state['section']) {
      case $this->my_name():
        $form_option = array('#tree' => TRUE);
        $options = $this->get_option($this->my_name());

        $form_option['help'] = array(
          '#type' => 'item',
          '#value' => t('Choose a usage type for each field in the search datasource. <strong>Content</strong> fields will be used to perform text searches. <strong>Attributes</strong> can be used to filter, sort or group the search results.')
        );

        $form_option['fields'] = array('#tree' => TRUE, '#theme' => 'searchlight_plugin_display_datasource');

        $fields = array_merge($this->get_fields(), $this->get_multivalues());
        foreach ($fields as $name => $info) {
          $form_option['fields'][$name] = array(
            '#title' => $info['label'],
            '#tree' => TRUE,
          );
          $usage_id = form_clean_id("attributes-{$name}-usage");
          $facet_id = form_clean_id("attributes-{$name}-facet");
          $settings_id = form_clean_id("attributes-{$name}-settings");

          // Datatype
          $form_option['fields'][$name]['datatype'] = array(
            '#type' => 'markup',
            '#value' => "<code>{$info['datatype']}</code>",
          );

          // Usage
          if (!empty($info['usage']) && $info['usage'] === 'multivalue') {
            $default_usage = isset($options['fields'][$name]['usage']) ? $options['fields'][$name]['usage'] : 'multivalue';
            $form_option['fields'][$name]['usage'] = array(
              '#type' => 'select',
              '#options' => array(FALSE => t('Disabled'), 'multivalue' => t('Multivalue')),
              '#default_value' => $default_usage,
              '#id' => $usage_id,
            );
          }
          else {
            $default_usage = ($info['datatype'] === 'text') ? 'content' : 'attribute';
            $default_usage = isset($options['fields'][$name]['usage']) ? $options['fields'][$name]['usage'] : $default_usage;
            $form_option['fields'][$name]['usage'] = array(
              '#type' => 'select',
              '#options' => array( 'content' => t('Content'), 'attribute' => t('Attribute')),
              '#default_value' => $default_usage,
              '#id' => $usage_id,
            );
          }

          // Facet
          $default_facet = in_array($default_usage, array('attribute', 'multivalue'), TRUE);
          $default_facet = isset($options['fields'][$name]['facet']) ? $options['fields'][$name]['facet'] : $default_facet;
          $form_option['fields'][$name]['facet'] = array(
            '#type' => 'checkbox',
            '#default_value' => $default_facet,
            '#process' => array('views_process_dependency'),
            '#dependency' => array($usage_id => array('attribute', 'multivalue')),
            '#id' => $facet_id,
          );

          // Per-datatype settings
          switch ($info['datatype']) {
            case 'timestamp':
              $form_option['fields'][$name]['granularity'] = array(
                '#type' => 'select',
                '#options' => array(
                  'day' => t('Day'),
                  'month' => t('Month'),
                  'year' => t('Year'),
                ),
                '#default_value' => isset($options['fields'][$name]['granularity']) ? $options['fields'][$name]['granularity'] : 'month',
                '#process' => array('views_process_dependency'),
                '#dependency' => array($facet_id => array(TRUE)),
                '#id' => $settings_id,
              );
              break;
            default:
              // Placeholder for now.
              $form_option['fields'][$name]['settings'] = array('#type' => 'markup', '#value' => '');
              break;
          }
        }

        // In reality, these are multivalue fields that are generated
        // programmatically.
        if ($this->view->base_table === 'node') {
          $form_option['node_access'] = array(
            '#title' => t('Node access'),
            '#description' => t('Include node access information in datasource'),
            '#type' => 'checkbox',
            '#default_value' => isset($options['node_access']) ? $options['node_access'] : TRUE,
          );
        }

        $form[$this->my_name()] = $form_option;
        break;
    }
  }

  /**
   * Perform any necessary changes to the form values prior to storage.
   * There is no need for this function to actually store the data.
   */
  function options_submit($form, &$form_state) {
    parent::options_submit($form, $form_state);
    switch ($form_state['section']) {
      case $this->my_name():
        $this->set_option($form_state['section'], $form_state['values'][$form_state['section']]);
        searchlight_invalidate_cache();
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
    foreach (array_keys($this->force_options()) as $key) {
      if (isset($options[$key])) {
        unset($options[$key]);
      }
    }
    $categories['basic'] = array('title' => t('Datasource settings'));
    $options[$this->my_name()] = array(
      'category' => 'basic',
      'title' => t('Datasource settings'),
      'value' => t('Settings'),
    );
  }

  /**
   * Enforce the options described in ->force_options().
   */
  function option_definition() {
    $options = parent::option_definition();
    $options[$this->my_name()] = array('default' => array());
    foreach ($this->force_options() as $key => $value) {
      if (isset($options[$key]['default'], $value)) {
        $options[$key]['default'] = $value;
      }
      if (isset($options['defaults']['default'][$key])) {
        $options['defaults']['default'][$key] = FALSE;
      }
    }
    return $options;
  }

  /**
   * Validate handler.
   */
  function validate() {
    $errors = parent::validate();
    if ($this->get_option('sorts') || $this->get_option('arguments')) {
      $errors[] = t('Searchlight datasource displays may not use arguments or sorts.');
    }
    return $errors;
  }
}
