<?php

class searchlight_plugin_display_datasource extends views_plugin_display {
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
  protected function get_datatype($handler) {
    $attribute = array();
    $schema = drupal_get_schema($handler->table);
    if ($schema && isset($handler->real_field, $schema['fields'][$handler->real_field]) && $handler->real_field != $handler->view->base_field) {
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
      if (isset($map[$schema['fields'][$handler->real_field]['type']])) {
        $column_type = $map[$schema['fields'][$handler->real_field]['type']];
        if ($column_type === 'int' && strpos($class, 'date') !== FALSE) {
          return 'timestamp';
        }
        return isset($map[$column_type]) ? $map[$column_type] : FALSE;
      }
    }
    return FALSE;
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
    $options = $this->get_option('searchlight_datasource');
    $this->view->build();
    $datasource = array(
      'attributes' => $options['attributes'],
      'base_table' => $this->view->base_table,
      'id' => searchlight_get_datasource_id($this->view->base_table),
      'view' => $this->view,
    );
    return $datasource;
  }

  /**
   * Provide the default form for setting options.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    switch ($form_state['section']) {
      case 'searchlight_datasource':
        $form_option = array('#tree' => TRUE);
        $options = $this->get_option('searchlight_datasource');

        $form_option['help'] = array(
          '#type' => 'item',
          '#value' => t('Choose a usage type for each field in the search datasource. <strong>Content</strong> fields will be used to perform text searches. <strong>Attributes</strong> can be used to filter, sort or group the search results.')
        );

        $handlers = $this->display->handler->get_handlers('field');
        $form_option['attributes'] = array('#tree' => TRUE, '#theme' => 'searchlight_plugin_display_datasource');

        // @TODO: Rewrite this to support $handler->additional_fields
        foreach ($handlers as $field => $handler) {
          if (($handler->real_field !== $this->view->base_field) && ($datatype = $this->get_datatype($handler))) {
            $name = "{$handler->table}.{$handler->real_field}";
            $form_option['fields'][$name] = array(
              '#title' => $handler->ui_name(),
              '#tree' => TRUE,
            );
            $elem = str_replace('.', '-', $name);
            $usage_id = form_clean_id("attributes-{$elem}-usage");
            $facet_id = form_clean_id("attributes-{$elem}-facet");

            // Datatype
            $form_option['fields'][$name]['datatype'] = array(
              '#type' => 'value',
              '#value' => $datatype,
            );
            $form_option['fields'][$name]['_datatype'] = array(
              '#type' => 'markup',
              '#value' => "<code>{$datatype}</code>",
            );

            // Usage
            $default_usage = ($datatype === 'text') ? 'content' : 'attribute';
            $default_usage = isset($options['fields'][$name]['usage']) ? $options['fields'][$name]['usage'] : $default_usage;
            $form_option['fields'][$name]['usage'] = array(
              '#type' => 'select',
              '#options' => array('content' => t('Content'), 'attribute' => t('Attribute')),
              '#default_value' => $default_usage,
              '#id' => $usage_id,
            );

            // Facet
            $default_facet = ($default_usage === 'attribute');
            $default_facet = isset($options['field'][$name]['facet']) ? $options['field'][$name]['facet'] : $default_facet;
            $form_option['fields'][$name]['facet'] = array(
              '#type' => 'checkbox',
              '#default_value' => $default_facet,
              '#process' => array('views_process_dependency'),
              '#dependency' => array($usage_id => array('attribute')),
              '#id' => $facet_id,
            );
          }
        }

        // Multi-valued attributes.
        foreach (searchlight_get_usable_views($this->view->base_table, 'searchlight_attribute') as $view) {
          foreach ($view->display as $display) {
            if ($display->display_plugin === 'searchlight_attribute') {
              $view = views_get_view($view->name);

              $view->set_display($display->id);
              $attribute = $view->execute_display();
              $name = "{$attribute['table']}.{$attribute['field']}";

              $elem = str_replace('.', '-', $name);
              $usage_id = form_clean_id("attributes-{$elem}-usage");
              $facet_id = form_clean_id("attributes-{$elem}-facet");

              $form_option['attributes'][$name] = array(
                '#title' => t('@display (@view)', array('@display' => $display->display_title, '@view' => $view->name)),
                '#tree' => TRUE,
              );

              // Usage
              $default_usage = 'attribute';
              $default_usage = isset($options['attributes'][$name]['usage']) ? $options['attributes'][$name]['usage'] : $default_usage;
              $form_option['attributes'][$name]['usage'] = array(
                '#type' => 'select',
                '#options' => array(0 => t('<'. t('Disabled') .'>'), 'attribute' => t('Multi-valued attribute')),
                '#default_value' => $default_usage,
                '#id' => $usage_id,
              );

              // Facet
              $default_facet = ($default_usage === 'attribute');
              $default_facet = isset($options['attributes'][$name]['facet']) ? $options['attributes'][$name]['facet'] : $default_facet;
              $form_option['attributes'][$name]['facet'] = array(
                '#type' => 'checkbox',
                '#default_value' => $default_facet,
                '#dependency' => array($usage_id => array('attribute')),
                '#id' => $facet_id,
              );

              // Store view
              $form_option['attributes'][$name]['view'] = array(
                '#type' => 'value',
                '#value' => array('view' => $view->name, 'display' => $display->id),
              );

              $view->destroy();
            }
          }
        }
        $form['searchlight_datasource'] = $form_option;
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
      case 'searchlight_datasource':
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
    $categories['basic'] = array('title' => t('Datasource settings'));
    $options['searchlight_datasource'] = array(
      'category' => 'basic',
      'title' => t('Datasource settings'),
      'value' => t('Settings'),
    );
    foreach (array_keys($this->force_options()) as $key) {
      if (isset($options[$key])) {
        unset($options[$key]);
      }
    }
  }

  /**
   * Enforce the options described in ->force_options().
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['searchlight_datasource'] = array('default' => array());
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
