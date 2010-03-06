<?php

class searchlight_plugin_display_attribute extends searchlight_plugin_display_datasource {
  /**
   * Write a SQL query with fully prefixed tables and replaced arguments.
   */
  protected function db_query($query, $args) {
    _db_query_callback($args, TRUE);
    $query = db_prefix_tables($query);
    $query = preg_replace_callback(DB_QUERY_REGEXP, '_db_query_callback', $query);
    return $query;
  }

  /**
   * Do not fully execute the view -- we only need the build the query object.
   * Generate an array representing a full description of this datasource for
   * Sphinx.
   */
  function execute() {
    $this->view->build();

    return;

    // Remove any fields that are not the id field or attribute field.
    $options = $this->get_option('searchlight_attribute');
    $query = drupal_clone($this->view->query);
    foreach ($query->fields as $alias => $query_field) {
      if ($query_field['field'] === $this->view->base_field && $query_field['table'] === $this->view->base_table) {
        continue;
      }
      if ($query_field['field'] === $options['attribute']) {
        $attribute = $alias;
        $table = $query_field['table'];
        $field = $query_field['field'];
      }
      else {
        unset($query->fields[$alias]);
      }
    }

    // Build MVA query.
    return array(
      'type' => 'sql_attr_multi',
      'name' => $attribute,
      'table' => $table,
      'field' => $field,
      'view' => $this->view->name,
      'display' => $this->view->display_handler->display->id,
      'mva_label_field' => $options['facet'],
      'mva_type' => 'uint',
      'mva_source' => 'query',
      'mva_query' => $this->db_query($query->query(), $query->get_where_args()),
    );
  }

  /**
   * Provide the default form for setting options.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    switch ($form_state['section']) {
      case 'searchlight_attribute':
        $form['searchlight_attribute'] = array('#tree' => TRUE);
        $options = $this->get_option('searchlight_attribute');

        $handlers = $this->display->handler->get_handlers('field');
        if (!empty($handlers)) {
          // @TODO: Because of the complexity of table aliasing when relationships
          // are involved, we currently only use the field name, not table.field
          // to identify our key fields.
          $field_options = array();
          foreach ($handlers as $handler) {
            if (!empty($handler->real_field)) {
              $field_options[$handler->real_field] = $handler->real_field;
            }
            if (!empty($handler->additional_fields)) {
              foreach ($handler->additional_fields as $field) {
                $field_options[$field] = $field;
              }
            }
          }
          $form['searchlight_attribute']['attribute'] = array(
            '#type' => 'select',
            '#title' => t('Attribute'),
            '#description' => t('The selected field will be provided as a multivalue attribute for use with any @basetable datasources.', array('@basetable' => $this->view->base_table)),
            '#options' => $field_options,
            '#default_value' => isset($options['attribute']) ? $options['attribute'] : NULL,
          );
          $form['searchlight_attribute']['facet'] = array(
            '#type' => 'select',
            '#title' => t('Facet display'),
            '#description' => t('The selected field will be used to display labels for this attribute when used as facet.'),
            '#options' => $field_options,
            '#default_value' => isset($options['facet']) ? $options['facet'] : NULL, 
          );
        }
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
      case 'searchlight_attribute':
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
    $categories['basic'] = array('title' => t('Searchlight attribute'));
    $options['searchlight_attribute'] = array(
      'category' => 'basic',
      'title' => t('Searchlight attribute'),
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
    $options['searchlight_attribute'] = array('default' => array());

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
}
