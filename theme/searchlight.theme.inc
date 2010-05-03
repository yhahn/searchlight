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
 * Preprocessor for theme('searchlight_solr_schema').
 */
function template_preprocess_searchlight_solr_schema(&$vars) {
  $defaults = array(
    'indexed' => 'true',
    'stored' => 'false',
    'multiValued' => 'false'
  );
  foreach($vars['datasource']['schema'] as $id => $field) {
    // Populate defaults.
    foreach($defaults as $k => $v) {
      if (!isset($field[$k])) {
        $vars['datasource']['schema'][$id][$k] = $v;
      }
    }
    // Normalize boolean values
    foreach ($field as $attribute => $value) {
      if (is_bool($value)) {
        $vars['datasource']['schema'][$id][$attribute] = ($value === TRUE ? 'true' : 'false');
      }
    }
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
  $output = theme('table', array(t('Field'), t('Datatype'), t('Usage'), t('Show facet'), ''), $rows);
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
  return "<div class='searchlight-facet'>{$item['title']} <span class='remove'>{$item['link']}</span></div>";
}

/**
 * Generates the main datasource admin page.
 */
function theme_searchlight_admin_list($form) {
  drupal_add_css(drupal_get_path("module", "searchlight") ."/searchlight.css");

  // Generate listing of existing datasources
  foreach ($form['#objects'] as $object) {
    if ($object instanceof SearchlightDatasource) {
      $extra = $object->base_table;
      $path = 'datasource';
      $name_label = t('Datasource');
      $extra_label = t('Base table');
    }
    else if ($object instanceof SearchlightEnvironment) {
      $extra = $object->datasource;
      $path = 'environment';
      $name_label = t('Environment');
      $extra_label = t('Datasource');
    }

    $row = array();

    // Info
    $storage = t('Default');
    if ($object->export_type & EXPORT_IN_DATABASE) {
      $storage = $object->export_type & EXPORT_IN_CODE ? t('Overridden') : t('Normal');
    }
    $row[] = array('data' => $object->name, 'class' => 'object-name');
    $row[] = array('data' => $extra, 'class' => 'object-extra');
    $row[] = array('data' => $storage, 'class' => 'object-storage');

    // Actions
    $links = array(
      'edit'    => l(t('Edit'), "admin/settings/search/{$path}/list/{$object->name}"),
      'delete'  => l(t('Delete'), "admin/settings/search/{$path}/list/{$object->name}/delete"),
      'revert'  => l(t('Revert'), "admin/settings/search/{$path}/list/{$object->name}/revert"),
    );
    foreach (array_keys($links) as $key) {
      if (!searchlight_task_access($object, $key)) {
        unset($links[$key]);
      }
    }
    $row[] = array('data' => implode(' | ', $links), 'class' => 'object-links');
    $rows[] = $row;
  }
  $row = array();
  foreach (element_children($form['new']) as $key) {
    if ($form['new'][$key]['#type'] === 'submit') {
      $row[] = array('data' => drupal_render($form['new'][$key]), 'colspan' => 2);
    }
    else {
      $row[] = drupal_render($form['new'][$key]);
    }
  }
  $rows[] = $row;
  $output = theme('table', array($name_label, $extra_label, t('Storage'), t('Operations')), $rows, array('class' => 'object-admin'));
  $output .= drupal_render($form);
  return $output;
}

/**
 * Theme function for searchlight datasource display form.
 */
function theme_searchlight_admin_datasource_fields($form) {
  $output = '';
  $rows = array();

  // New fields header.
  if (!empty($form['new'])) {
    $rows[] = array(array('data' => t('Add field'), 'colspan' => 4, 'header' => TRUE));
    $rows[] = array(
      array('data' => drupal_render($form['new']['field']), 'colspan' => 3),
      array('data' => drupal_render($form['new']['add'])),
    );
  }

  // Existing fields.
  $rows[] = array(
    theme('table_select_header_cell') + array('header' => TRUE),
    array('data' => t('Field'), 'header' => TRUE),
    array('data' => t('Datatype'), 'header' => TRUE),
    array('data' => t('Usage'), 'header' => TRUE)
  );
  foreach (element_children($form['fields']) as $key) {
    if ($key !== 'new') {
      $row = array();
      foreach (element_children($form['fields'][$key]) as $cell) {
        if (isset($form['fields'][$key][$cell]['#type']) && !in_array($form['fields'][$key][$cell]['#type'], array('hidden', 'value'))) {
          $row[] = drupal_render($form['fields'][$key][$cell]);
        }
      }
      $rows[] = $row;
    }
  }
  if (!element_children($form['fields'])) {
    $rows[] = array(array('data' => t('No fields found.'), 'colspan' => 4));
  }

  // Remove fields footer.
  if (!empty($form['remove'])) {
    $rows[] = array(array('data' => drupal_render($form['remove']), 'colspan' => 4, 'header' => TRUE));
  }

  $output = theme('table', array(), $rows, array(), $form['#title']);
  $output .= drupal_render($form);
  return $output;
}

/**
 * Theme function for searchlight environment form.
 */
function theme_searchlight_admin_environment($form) {
  drupal_add_tabledrag('searchlight-environment', 'order', 'sibling', 'searchlight-environment-weight', NULL, NULL, TRUE);
  $output = '';
  $output .= drupal_render($form['name']);

  $rows = array();
  $rows[] = array(
    array('data' => drupal_render($form['datasource']['datasource']), 'colspan' => 3),
    array('data' => drupal_render($form['datasource']['update']), 'colspan' => 2),
  );
  $rows[] = array(
    array('data' => t('Enabled'), 'header' => TRUE),
    array('data' => t('Facet'), 'header' => TRUE),
    array('data' => t('Custom label'), 'header' => TRUE),
    array('data' => t('Weight'), 'header' => TRUE),
    array('data' => t('Items to show'), 'header' => TRUE),
  );
  uasort($form['facets'], 'element_sort');
  foreach (element_children($form['facets']) as $key) {
    $row = array();
    foreach (element_children($form['facets'][$key]) as $cell) {
      if ($cell === 'weight') {
        $form['facets'][$key][$cell]['#attributes']['class'] = 'searchlight-environment-weight';
      }
      $row[] = drupal_render($form['facets'][$key][$cell]);
    }
    $rows[] = array('data' => $row, 'class' => 'draggable');
  }
  $output .= theme('table', array(), $rows, array('id' => 'searchlight-environment'));
  $output .= drupal_render($form);
  return $output;
}
