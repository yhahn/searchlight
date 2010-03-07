<?php

class searchlight_plugin_display_attribute extends searchlight_plugin_display_datasource {
  /**
   * Return the name of this plugin that will be used as the storage key
   * for settings, other identifiers.
   */
  protected function my_name() {
    return 'searchlight_multivalue';
  }

  /**
   * Do not fully execute the view -- we only need the build the query object.
   * Generate an array representing a full description of this multivalue.
   */
  function execute() {
    $options = $this->get_option($this->my_name());
    $this->view->build();
    $multivalue = $options + array(
      'base_table' => $this->view->base_table,
      'view' => $this->view,
    );
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
          $subform['field'] = array(
            '#type' => 'select',
            '#title' => t('Attribute field'),
            '#description' => t('The selected field will be provided as a multivalue attribute for use with any @basetable datasources.', array('@basetable' => $this->view->base_table)),
            '#options' => $field_options,
            '#default_value' => isset($options['field']) ? $options['field'] : NULL,
          );
          $subform['facet'] = array(
            '#type' => 'select',
            '#title' => t('Facet display'),
            '#description' => t('The selected field will be used to display labels for this attribute when used as facet.'),
            '#options' => $field_options,
            '#default_value' => isset($options['facet']) ? $options['facet'] : NULL, 
          );
        }

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
    $categories['basic'] = array('title' => t('Searchlight attribute'));
    $options['searchlight_multivalue'] = array(
      'category' => 'basic',
      'title' => t('Searchlight attribute'),
      'value' => t('Settings'),
    );
  }
}
