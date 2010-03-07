<?php

/**
 * Format a conf value for nice printing in the sphinx.conf file.
 */
function searchlight_sphinx_conf_print($value) {
  return "{$value}\n";
}

/**
 * Preprocessor for theme('searchlight_sphinx_conf').
 */
function template_preprocess_searchlight_sphinx_conf(&$vars) {
  foreach ($vars['datasources'] as $id => $datasource) {
    foreach ($datasource['conf'] as $key => $value) {
      $vars['datasources'][$id]['conf'][$key] = searchlight_sphinx_conf_print($value);
    }
    foreach ($datasource['index'] as $key => $value) {
      $vars['datasources'][$id]['index'][$key] = searchlight_sphinx_conf_print($value);
    }
  }
  foreach ($vars['searchd'] as $key => $value) {
    $vars['searchd'][$key] = searchlight_sphinx_conf_print($value);
  }
}

/**
 * Theme function for searchlight datasource display form.
 */
function theme_searchlight_plugin_display_datasource($form) {
  $output = '';
  $rows = array();
  foreach (element_children($form) as $key) {
    $row = array();
    $row[] = $form[$key]['#title'];
    foreach (element_children($form[$key]) as $cell) {
      if (isset($form[$key][$cell]['#type']) && !in_array($form[$key][$cell]['#type'], array('hidden', 'value'))) {
        $row[] = drupal_render($form[$key][$cell]);
      }
    }
    $rows[] = $row;
  }
  $output = theme('table', array(t('Field'), t('Datatype'), t('Usage'), t('Show facet')), $rows);
  $output .= drupal_render($form);
  return $output;
}
