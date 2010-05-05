<?php

/**
 * Datasource management form.
 */
function searchlight_admin_environment($form_state) {
  $form = array(
    '#theme' => 'searchlight_admin_list',
    '#objects' => array(searchlight_environment_load(NULL, TRUE)),
  );
  $form['new']['name'] = array(
    '#title' => t('Name'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#maxlength' => 255,
    '#size' => 15,
    '#element_validate' => array('searchlight_admin_environment_validate_name'),
  );

  $form['new']['view_display'] = array(
    '#title' => t('View'),
    '#type' => 'select',
    '#options' => searchlight_admin_environment_view_displays(),
  );

  $form['new']['submit'] = array(
    '#value' => t('Create new environment'),
    '#type' => 'submit',
    '#submit' => array('searchlight_admin_environment_new'),
  );
  return $form;
}

/**
 * Retrieve an array of Views that can be used with searchlight environments.
 */
function searchlight_admin_environment_view_displays() {
  // Gather all views that have the active facets filter.
  $views = views_get_all_views();
  $usable = array();
  foreach ($views as $view) {
    foreach ($view->display as $display) {
      if ($display->display_plugin === 'page') {
        $view->set_display($display->id);
        $filters = $view->display_handler->get_option('filters');
        if (!empty($filters)) {
          foreach ($filters as $filter) {
            if ($filter['table'] === 'searchlight' && $filter['field'] === 'facets') {
              $usable[$view->base_table]["{$view->name}:{$display->id}"] = "{$view->name}: {$display->display_title}";
            }
          }
        }
      }
    }
  }
  return $usable;
}

/**
 * Validate environment name values.
 */
function searchlight_admin_environment_validate_name($element, &$form_state) {
  // Check for string identifier sanity
  if (!preg_match('!^[a-z0-9_-]+$!', $element['#value'])) {
    form_set_error('name', t('The environment name can only consist of lowercase letters, dashes, underscores, and numbers.'));
  }
  // Check for name collision
  else if ($exists = searchlight_environment_load($element['#value'], TRUE)) {
    form_set_error('name', t('A environment with this name already exists. Please choose another name or delete the existing environment before creating a new one.'));
  }
}

/**
 * Searchlight datasource form submit handler.
 */
function searchlight_admin_environment_new(&$form, &$form_state) {
  $environment = searchlight_environment_new($form_state['values']['name'], $form_state['values']['view_display']);
  if (searchlight_environment_save($environment)) {
    drupal_set_message(t('Saved environment %name.', array('%name' =>  $environment->name)));
  }
  else {
    drupal_set_message(t('Could not save environment %name.', array('%name' => $environment->name)), 'error');
  }
}

/**
 * Edit form for environment.
 */
function searchlight_admin_environment_edit($form_state, $environment) {
  $form = array();
  $form['#environment'] = $environment;
  $form['#environment_name'] = $environment->name;
  $environment->optionsForm($form, $form_state);

  $form['buttons']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form['buttons']['cancel'] = array(
    '#type' => 'markup',
    '#value' => l(t('Cancel'), 'admin/settings/search/environment'),
  );
  return $form;
}

/**
 * Submit handler for searchlight_admin_environment_edit().
 */
function searchlight_admin_environment_edit_submit(&$form, $form_state) {
  if (!empty($form['#environment'])) {
    $form['#environment']->optionsSubmit($form, $form_state);
    searchlight_environment_save($form['#environment']);
    drupal_set_message(t('Environment @environment saved.', array('@environment' => $form['#environment']->name)));
  }
}

/**
 * Datasource management form.
 */
function searchlight_admin_datasource($form_state) {
  views_include('admin');
  $base_tables = views_fetch_base_tables();

  $form = array(
    '#theme' => 'searchlight_admin_list',
    '#grouped' => TRUE,
    '#objects' => array(),
    'active' => array('#tree' => TRUE),
  );
  foreach (searchlight_datasource_load(NULL, TRUE) as $datasource) {
    $group = $base_tables[$datasource->base_table]['title'];
    // Add in objects.
    $form['#objects'][$group][] = $datasource;

    // Add in active datasource radio option.
    if (!isset($form['active'][$datasource->base_table])) {
      $form['active'][$datasource->base_table] = array(
        '#type' => 'radios',
        '#options' => array(),
        '#default_value' => variable_get("searchlight_datasource_{$datasource->base_table}", "searchlight_{$datasource->base_table}"),
      );
    }
    $form['active'][$datasource->base_table]['#options'][$datasource->name] = $datasource->name;
  }

  $form['new']['name'] = array(
    '#title' => t('Name'),
    '#type' => 'textfield',
    '#maxlength' => 255,
    '#size' => 15,
  );

  $options = array();
  foreach ($base_tables as $table => $info) {
    $options[$table] = $info['title'];
  }
  $form['new']['base_table'] = array(
    '#title' => t('Base table'),
    '#type' => 'select',
    '#options' => $options,
  );
  $form['new']['submit'] = array(
    '#value' => t('Create new datasource'),
    '#type' => 'submit',
    '#validate' => array('searchlight_admin_datasource_new_validate'),
    '#submit' => array('searchlight_admin_datasource_new'),
  );

  $form['buttons']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#submit' => array('searchlight_admin_datasource_save'),
  );
  $form['buttons']['cancel'] = array(
    '#type' => 'markup',
    '#value' => l(t('Cancel'), 'admin/settings/search/datastore'),
  );
  return $form;
}

/**
 * Validate datasource creation values.
 */
function searchlight_admin_datasource_new_validate(&$form, &$form_state) {
  $name = $form_state['values']['name'];
  // Check that the name field is populated
  if (empty($name)) {
    form_set_error('name', t('!name field is required.', array('!name' => $form['new']['name']['#title'])));
  }
  // Check for string identifier sanity
  else if (!preg_match('!^[a-z0-9_-]+$!', $name)) {
    form_set_error('name', t('The datasource name can only consist of lowercase letters, dashes, underscores, and numbers.'));
  }
  // Check for name collision
  else if ($exists = searchlight_datasource_load($name, TRUE)) {
    form_set_error('name', t('A datasource with this name already exists. Please choose another name or delete the existing datasource before creating a new one.'));
  }
}

/**
 * Searchlight datasource creation submit handler.
 */
function searchlight_admin_datasource_new(&$form, &$form_state) {
  $datasource = searchlight_datasource_new($form_state['values']['name'], $form_state['values']['base_table']);
  if (searchlight_datasource_save($datasource)) {
    drupal_set_message(t('Created datasource %name.', array('%name' =>  $datasource->name)));
  }
  else {
    drupal_set_message(t('Could not save datasource %name.', array('%name' => $datasource->name)), 'error');
  }
}

/**
 * Searchlight active datasource submit handler.
 */
function searchlight_admin_datasource_save(&$form, &$form_state) {
  foreach ($form_state['values']['active'] as $base_table => $datasource) {
    if (!empty($datasource)) {
      $current = variable_get("searchlight_datasource_{$base_table}", "searchlight_{$base_table}");
      variable_set('searchlight_datasource_'. $base_table, $datasource);
      drupal_set_message(t('New active datasource settings have been saved.'), 'status', FALSE);

      // If active datasource settings have changed a rebuild is necessary.
      if ($current != $datasource) {
        drupal_set_message(t('Active datasource settings have changed. Please rebuild your configuration and search index.'), 'warning', FALSE);
      }
    }
  }
}

/**
 * Edit form for datasource.
 */
function searchlight_admin_datasource_edit($form_state, $datasource) {
  $form = array();
  $form['#datasource'] = $datasource;
  $form['#datasource_name'] = $datasource->name;
  $datasource->init();
  $datasource->optionsForm($form, $form_state);

  $form['buttons']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form['buttons']['cancel'] = array(
    '#type' => 'markup',
    '#value' => l(t('Cancel'), 'admin/settings/search/datastore'),
  );
  return $form;
}

/**
 * Submit handler for searchlight_admin_datasource_edit().
 */
function searchlight_admin_datasource_edit_submit(&$form, $form_state) {
  if (!empty($form['#datasource'])) {
    $form['#datasource']->optionsSubmit($form, $form_state);
    searchlight_datasource_save($form['#datasource']);
    drupal_set_message(t('Datasource @datasource saved. The index for this datasource needs to be rebuilt.', array('@datasource' => $form['#datasource']->name)));
  }
}

/**
 * AHAH callback & processor for datasource editor.
 */
function searchlight_admin_datasource_ahah($clicked_id, $element = 'fields') {
  $form_state = array();
  $form_state['clicked_button']['#id'] = $clicked_id;
  $form_state['values'] = $_POST;
  $form = form_get_cache($_POST['form_build_id'], $form_state);
  if ($form) {
    $datasource = searchlight_datasource_load($form['#datasource_name']);
    $datasource->optionsSubmit($form, $form_state);
    searchlight_datasource_save($datasource);

    $form['#datasource'] = $datasource;
    $datasource->optionsForm($form, $form_state);
    form_set_cache($_POST['form_build_id'], $form, $form_state);

    // Build and render the new element, then return it in JSON format.
    $form_state = array();
    $form['#post'] = array();
    $form = form_builder($form['form_id']['#value'], $form, $form_state);
    $output = drupal_render($form[$element]);
    drupal_json(array('status' => TRUE, 'data' => $output));
  }
  else {
    drupal_json(array('status' => FALSE, 'data' => ''));
  }
  exit();
}

/**
 * Confirmation form for datasource actions.
 */
function searchlight_admin_confirm(&$form_state, $type, $object, $op) {
  switch ($type) {
    case 'searchlight_datasource':
      $type_name = t('datasource');
      break;
    case 'searchlight_environment':
      $type_name = t('environment');
      break;
  }
  $form = array();
  $form['type'] = array('#type' => 'value', '#value' => $type);
  $form['object'] = array('#type' => 'value', '#value' => $object);
  $form['action'] = array('#type' => 'value', '#value' => $op);
  switch ($op) {
    case 'revert':
      $action = t('revert');
      $message = t('This action will permanently remove any customizations made to this @type.', array('@type' => $type_name));
      break;
    case 'delete':
      $action = t('delete');
      $message = t('This action will remove this @type permanently from your site.', array('@type' => $type_name));
      break;
    case 'disable':
      $action = t('disable');
      $message = '';
      break;
    case 'enable':
      $action = t('enable');
      $message = '';
      break;
  }
  $form = confirm_form($form,
    t('Are you sure you want to !action the @type %title?', array('%title' => $object->name, '@type' => $type_name, '!action' => $action)),
    'admin/settings/search',
    $message,
    drupal_ucfirst($action), t('Cancel')
  );
  return $form;
}

/**
 * Submit handler for the searchlight_admin_datasource_confirm.
 */
function searchlight_admin_confirm_submit($form, &$form_state) {
  ctools_include('export');
  $object = $form_state['values']['object'];
  switch ($form_state['values']['action']) {
    case 'revert':
    case 'delete':
      switch ($form_state['values']['type']) {
        case 'searchlight_datasource':
          searchlight_datasource_delete($object);
          $form_state['redirect'] = 'admin/settings/search/datasource';
          break;
        case 'searchlight_environment':
          searchlight_environment_delete($object);
          $form_state['redirect'] = 'admin/settings/search/environment';
          break;
      }
      break;
  }
}

/**
 * System settings form for Searchlight.
 */
function searchlight_admin_backend($form_state) {
  drupal_add_js(drupal_get_path('module', 'searchlight') .'/searchlight.js');

  $form = array();

  // Backend selection.
  $form['backend'] = array(
    '#tree' => FALSE,
    '#type' => 'fieldset',
    '#title' => t('Backend'),
    '#description' => t('Choose a search backend to use with Searchlight.'),
  );
  $form['backend']['searchlight_backend'] = array(
    '#type' => 'select',
    '#options' => array(0 => '< '. t('Choose a backend') .' >'),
    '#default_value' => variable_get('searchlight_backend', 'sphinx'),
    '#attributes' => array(
      'class' => 'searchlight-backend-select',
    ),
  );
  foreach (searchlight_registry('backend') as $key => $info) {
    $form['backend']['searchlight_backend']['#options'][$key] = $info['title'];
    $backend = searchlight_get_backend($key);
    $form["searchlight_backend_{$key}"] = $backend->settingsForm(variable_get("searchlight_backend_{$key}", array()));
    $form["searchlight_backend_{$key}"]['#tree'] = TRUE;
    $form["searchlight_backend_{$key}"]['#title'] = $info['title'];
    $form["searchlight_backend_{$key}"]['#type'] = 'fieldset';
    $form["searchlight_backend_{$key}"]['#attributes'] = array(
      'class' => "searchlight-backend-settings searchlight-backend-{$key}",
    );
  }
  $form = system_settings_form($form);
  array_unshift($form['#submit'], 'searchlight_admin_backend_submit');
  return $form;
}

/**
 * Submit handler that runs prior to system_settings_form_submit().
 */
function searchlight_admin_backend_submit(&$form, &$form_state) {
  $current = variable_get('searchlight_backend', 'sphinx');
  if ($current != $form_state['values']['searchlight_backend']) {
    // Switch backends. This must occur *before* we invalidate the index.
    variable_set('searchlight_backend', $form_state['values']['searchlight_backend']);
    searchlight_invalidate_index();
    drupal_set_message(t('Searchlight backend settings have changed. Please rebuild your configuration and search index.'), 'warning', FALSE);
  }
}
