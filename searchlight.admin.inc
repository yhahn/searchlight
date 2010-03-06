<?php

/**
 * System settings form for Sphinx Views.
 */
function sphinx_views_settings_form($form_state) {
  $views = variable_get('sphinx_views', array());
  $form = array();
  $form['sphinx_views'] = array(
    '#tree' => TRUE,
    '#type' => 'fieldset',
    '#title' => t('Sphinx views'),
    '#description' => t('Choose a view to use as the search configuration for each table.'),
  );
  views_include('admin');
  foreach (views_fetch_base_tables() as $base_table => $info) {
    $options = drupal_map_assoc(array_keys(sphinx_views_get_usable_views($base_table, 'sphinx_datasource')));
    if (!empty($options)) {
      $options = array(0 => '<'. t('Disabled') .'>') + $options;
      $form['sphinx_views'][$base_table] = array(
        '#title' => $info['title'],
        '#description' => $info['description'],
        '#weight' => $info['weight'],
        '#type' => 'select',
        '#options' => $options,
        '#default_value' => !empty($views[$base_table]) ? $views[$base_table] : 0,
      );
    }
  }
  $form = system_settings_form($form);
  return $form;
}
