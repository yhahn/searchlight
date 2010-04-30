<?php

class SearchlightEnvironment {
  var $name;
  var $datasource;
  var $facets;
  var $options;

  function __construct() {
    $this->name = '';
    $this->datasource = '';
    $this->facets = array();
    $this->options = array();
  }

  function optionsForm(&$form, &$form_state) {
    views_include('admin');
    views_include('form');

    $form['#theme'] = 'searchlight_admin_environment';

    $form['name'] = array(
      '#title' => t('Name'),
      '#type' => 'item',
      '#value' => $this->name,
    );

    $options = array();
    foreach (searchlight_datasource_load() as $datasource) {
      $options[$datasource->name] = $datasource->name;
    }
    $form['datasource']['#tree'] = TRUE;
    $form['datasource']['datasource'] = array(
      '#title' => t('datasource'),
      '#type' => 'select',
      '#default_value' => $this->datasource,
      '#options' => $options,
    );
    $form['datasource']['update'] = array(
      '#value' => t('Update datasource'),
      '#type' => 'submit',
      '#submit' => array('searchlight_admin_environment_edit_submit'),
    );

    $datasource = searchlight_datasource_load($this->datasource);
    if ($datasource) {
      $form['facets'] = array('#tree' => TRUE);
      foreach ($datasource->fields as $name => $field) {
        if (in_array($field['usage'], array('attribute', 'multivalue'), TRUE)) {
          $form['facets'][$name] = array(
            '#tree' => TRUE,
            '#weight' => isset($this->facets[$name]['weight']) ? $this->facets[$name]['weight'] : 0,
          );
          $form['facets'][$name]['enabled'] = array(
            '#type' => 'checkbox',
            '#default_value' => isset($this->facets[$name]['enabled']) ? $this->facets[$name]['enabled'] : TRUE,
          );
          $form['facets'][$name]['label'] = array(
            '#type' => 'markup',
            '#value' => $field['label'],
          );
          $form['facets'][$name]['weight'] = array(
            '#type' => 'weight',
            '#default_value' => isset($this->facets[$name]['weight']) ? $this->facets[$name]['weight'] : 0,
          );
          $form['facets'][$name]['items'] = array(
            '#type' => 'select',
            '#default_value' => isset($this->facets[$name]['items']) ? $this->facets[$name]['items'] : 5,
            '#options' => drupal_map_assoc(range(1, 20)),
          );
        }
      }
    }
  }

  function optionsSubmit(&$form, &$form_state) {
    switch ($form_state['clicked_button']['#id']) {
      case 'edit-datasource-update':
        $this->datasource = $form_state['values']['datasource']['datasource'];
        break;
      case 'edit-save':
        $this->facets = $form_state['values']['facets'];
        // $this->options = $form_state['values']['options'];
        break;
    }
  }
}
