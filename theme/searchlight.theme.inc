<?php

/**
 * Preprocessor for theme('searchlight_sphinx_conf').
 */
function template_preprocess_searchlight_sphinx_conf(&$vars) {
  foreach ($vars['datasources'] as $id => $datasource) {
    foreach ($datasource['conf'] as $key => $value) {
      $vars['datasources'][$id]['conf'][$key] = "$value\n";
    }
    foreach ($datasource['index'] as $key => $value) {
      $vars['datasources'][$id]['index'][$key] = "$value\n";
    }
  }
  foreach ($vars['searchd'] as $key => $value) {
    $vars['searchd'][$key] = "$value\n";
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

/**
 * Theme function for a facet.
 */
function theme_searchlight_facet($facet) {
  return theme('item_list', $facet['items'], $facet['label']);
}

/**
 * Theme function for an individual (inactive) facet link.
 */
function theme_searchlight_facet_link($field, $item) {
  return "<div class='searchlight-facet'>{$item['link']} <span class='count'>{$item['count']}</span></div>";
}

/**
 * Theme function for an individual (active) facet link.
 */
function theme_searchlight_facet_active($field, $item) {
  return "<div class='searchlight-facet'>{$item['title']}</div>";
}
