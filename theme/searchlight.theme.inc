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
function theme_searchlight_admin_datasource($form) {
  drupal_add_css(drupal_get_path("module", "searchlight") ."/searchlight.css");

  // Generate listing of existing datasources
  foreach ($form['#datasources'] as $datasource) {
    $row = array();

    // Info
    $storage = t('Default');
    if ($datasource->export_type & EXPORT_IN_DATABASE) {
      $storage = $datasource->export_type & EXPORT_IN_CODE ? t('Overridden') : t('Normal');
    }
    $row[] = array('data' => $datasource->name, 'class' => 'datasource-name');
    $row[] = array('data' => $datasource->base_table, 'class' => 'datasource-base-table');
    $row[] = array('data' => $storage, 'class' => 'datasource-storage');

    // Actions
    $links = array(
      'edit'    => l(t('Edit'), "admin/settings/search/datasource/list/{$datasource->name}"),
      'delete'  => l(t('Delete'), "admin/settings/search/datasource/list/{$datasource->name}/delete"),
      'revert'  => l(t('Revert'), "admin/settings/search/datasource/list/{$datasource->name}/revert"),
    );
    foreach (array_keys($links) as $key) {
      if (!searchlight_datasource_task_access($datasource, $key)) {
        unset($links[$key]);
      }
    }
    $row[] = array('data' => implode(' | ', $links), 'class' => 'datasource-links');
    $rows[] = $row;
  }
  $rows[] = array(
    drupal_render($form['name']),
    drupal_render($form['base_table']),
    array('data' => drupal_render($form['submit']), 'colspan' => 2),
  );
  $output = theme('table', array(t('Datasource'), t('Base table'), t('Storage'), t('Operations')), $rows, array('class' => 'datasource-admin'));
  $output .= drupal_render($form);
  return $output;
}

/**
 * Theme function for searchlight datasource display form.
 */
function theme_searchlight_admin_datasource_edit($form) {
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
