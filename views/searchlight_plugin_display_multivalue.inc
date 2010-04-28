<?php

class searchlight_plugin_display_multivalue extends searchlight_plugin_display_datasource {
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
