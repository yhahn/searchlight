<?php

class SearchlightEnvironment {
  // Storage variables: saved to DB or code when an environment is saved.
  var $name;
  var $view_display;
  var $facets;
  var $options;

  // Working variables: populated at various init and other stages.
  var $datasource;
  var $enabled;
  var $active_values;

  // Class objects.
  var $plugins;
  var $query;
  var $view;

  function construct() {
    $this->name = isset($this->name) ? $this->name : '';
    $this->view_display = isset($this->view_display) ? $this->view_display : '';
    $this->facets = !empty($this->facets) ? $this->facets : array();
    $this->options = !empty($this->options) ? $this->options : array();
    $this->options = $this->options + array(
      'active_label' => t('Active search'),
      'facets_label' => t('Search facets'),
    );

    $this->enabled = array();
    $this->active_values = array();

    $this->plugins = array();
    $this->query = NULL;
    $this->view = NULL;
  }

  /**
   * Activate this environment using the values found (and parsed) from a PURL
   * modifier. See searchlight_environment_init().
   */
  function initValues($values) {
    $this->active_values = $values;

    // Remove the environment key which is for internal use only.
    if (isset($this->active_values['environment'])) {
      unset($this->active_values['environment']);
    }
  }

  /**
   * Initialize the environment view and all plugins.
   */
  function initView($view = NULL, $reset = FALSE) {
    if (!isset($this->view) || $reset) {
      $this->view = NULL;
      $this->datasource = NULL;

      // Build the view if none was explicitly provided.
      if (!isset($view)) {
        $view = searchlight_build_view($this->view_display);
      }

      // Check that the view matches the environment view and init.
      if ($view && "{$view->name}:{$view->current_display}" === $this->view_display) {
        $this->view = $view;
        $this->query = $view->query;
        $this->datasource = $view->query->datasource;
        $this->plugins = array();

        foreach ($this->getFacets() as $name => $facet) {
          $plugin = searchlight_get_facet($this->datasource, $name);
          $plugin->construct($this, $this->datasource->fields[$name], $this->getValue($name), $this->facets[$name]);
          $this->plugins[$name] = $plugin;
        }
      }
    }
    return !empty($this->view);
  }

  /**
   * Allow the environment to initialize plugins and allow them to modify the
   * Views query.
   */
  function query(&$query) {
    // If any facets are active treat it as though the result set is not empty.
    if ($this->getFacets(TRUE)) {
      $query->search_options['hideEmpty'] = FALSE;
    }
    foreach ($this->plugins as $name => $plugin) {
      $plugin->query($query);
    }
  }

  /**
   * Generate a set of renderable facet items.
   */
  function render($delta = 'active') {
    $rendered = array();
    foreach ($this->plugins as $name => $plugin) {
      if ($render = $plugin->render(drupal_clone($this->query), $delta)) {
        $rendered[$name] = array('label' => $plugin->label($delta), 'items' => $render);
      }
    }
    return $rendered;
  }

  /**
   * Retrieve the active value for a given facet.
   */
  function getValue($key) {
    $facets = $this->getFacets();
    if (isset($this->active_values[$key]) && isset($facets[$key])) {
      return $this->active_values[$key];
    }
    return NULL;
  }

  /**
   * Build a block suitable for return by hook_block('view').
   */
  function getBlock($delta) {
    if ($this->initView()) {
      $output = '';
      foreach ($this->render($delta) as $rendered) {
        $output .= theme('searchlight_facet', $rendered);
      }
      if (!empty($output)) {
        return array(
          'subject' => filter_xss_admin($this->options["{$delta}_label"]),
          'content' => $output,
        );
      }
    }
    return array();
  }

  /**
   * Get the datasource associated with this environment's view.
   */
  function getDatasource($reset = FALSE) {
    // Adapted from searchlight_build_view. It should be possible to
    // retrieve a data source from a view without running it, untill then...
    $split = explode(':', $this->view_display);
    if (count($split) === 2 && $view = views_get_view($split[0])) {
      return searchlight_get_datasource($view->base_table);
    }
    return FALSE;
  }

  /**
   * Retrieve enabled facets for this environment.
   */
  function getFacets($active_only = FALSE) {
    if (empty($this->enabled)) {
      foreach ($this->facets as $name => $facet) {
        if (!empty($facet['enabled']) && $this->isValidFacet($name)) {
          // Set set #weight so element_sort can be used.
          $facet['#weight'] = isset($facet['weight']) ? $facet['weight'] : 0;
          $this->enabled[$name] = $facet;
        }
      }
      uasort($this->enabled, 'element_sort');
    }
    return $active_only ? array_intersect_key($this->enabled, $this->active_values) : $this->enabled;
  }

  /**
   * Options form for configuring this environment.
   */
  function optionsForm(&$form, &$form_state) {
    views_include('admin');
    views_include('form');

    $form['#theme'] = 'searchlight_admin_environment';

    $form['name'] = array(
      '#title' => t('Name'),
      '#type' => 'item',
      '#value' => $this->name,
    );

    $form['options']['#tree'] = TRUE;
    $form['options']['active_label'] = array(
      '#type' => 'textfield',
      '#title' => t('Active search label'),
      '#description' => t('Subject for the active search block in this environment.'),
      '#default_value' => $this->options['active_label'],
    );
    $form['options']['facets_label'] = array(
      '#type' => 'textfield',
      '#title' => t('Facets label'),
      '#description' => t('Subject for the facets block in this environment.'),
      '#default_value' => $this->options['facets_label'],
    );

    // Include searchlight.admin.inc for views displays function.
    module_load_include('inc', 'searchlight', 'searchlight.admin');
    $form['view_display']['#tree'] = TRUE;
    $form['view_display']['view_display'] = array(
      '#title' => t('View'),
      '#type' => 'select',
      '#default_value' => $this->view,
      '#options' => searchlight_admin_environment_view_displays(),
    );
    $form['view_display']['update'] = array(
      '#value' => t('Update view'),
      '#type' => 'submit',
      '#submit' => array('searchlight_admin_environment_edit_submit'),
    );

    $datasource = $this->getDatasource();
    if ($datasource) {
      $form['facets'] = array('#tree' => TRUE);
      $fields = $datasource->fields;
      $fields['search_query'] = array('label' => t('Search query'));
      foreach ($fields as $name => $field) {
        if ($this->isValidFacet($name)) {
          $form['facets'][$name] = array(
            '#tree' => TRUE,
            '#weight' => isset($this->facets[$name]['weight']) ? $this->facets[$name]['weight'] : 0,
          );
          $form['facets'][$name]['enabled'] = array(
            '#type' => 'checkbox',
            '#default_value' => isset($this->facets[$name]['enabled']) ? $this->facets[$name]['enabled'] : TRUE,
          );
          $form['facets'][$name]['ui_name'] = array(
            '#type' => 'markup',
            '#value' => $field['label'],
          );
          // @TODO:
          // It's terribly confusing that
          // - facet['label'] (Custom label for facet display)
          // - field['label'] (The Views UI name for a field)
          // are so very different.
          // Resolve this situation by renaming one or the other.
          $form['facets'][$name]['label'] = array(
            '#type' => 'textfield',
            '#size' => 20,
            '#default_value' => isset($this->facets[$name]['label']) ? $this->facets[$name]['label'] : '',
          );
          $form['facets'][$name]['weight'] = array(
            '#type' => 'weight',
            '#default_value' => isset($this->facets[$name]['weight']) ? $this->facets[$name]['weight'] : 0,
          );
          if ($name === 'search_query') {
            $form['facets'][$name]['items'] = array(
              '#type' => 'value',
              '#value' => 1,
            );
          }
          else {
            $form['facets'][$name]['items'] = array(
              '#type' => 'select',
              '#default_value' => isset($this->facets[$name]['items']) ? $this->facets[$name]['items'] : 5,
              '#options' => drupal_map_assoc(range(1, 20)),
            );
          }
        }
      }
    }
  }

  /**
   * Submit handler for options form.
   */
  function optionsSubmit(&$form, &$form_state) {
    switch ($form_state['clicked_button']['#id']) {
      case 'edit-view-display-update':
        $this->view_display = $form_state['values']['view_display']['view_display'];
        break;
      case 'edit-save':
        $this->facets = $form_state['values']['facets'];
        $this->options = $form_state['values']['options'];
        break;
    }
  }

  /**
   * Determine whether a given facet is valid.
   */
  function isValidFacet($name) {
    if ($name === 'search_query') {
      return TRUE;
    }
    else if ($datasource = $this->getDatasource()) {
      return (isset($datasource->fields[$name]) && in_array($datasource->fields[$name]['usage'], array('attribute', 'multivalue'), TRUE));
    }
  }

  /**
   * Get the URL options for the current set of active facets, adjusted using
   * one of the $op operations.
   *
   * 'add':    Add a facet value for the given key/value pair.
   * 'remove': Add a facet value for the given key/value pair.
   * 'active': Retain only active facets and drop any other query strings.
   */
  function getURLOptions($op = 'add', $key = NULL, $value = NULL) {
    $options = array();
    $modifier = array('environment' => $this->name);
    switch ($op) {
      case 'add':
        $modifier = $modifier + $this->active_values;
        $modifier[$key] = $value;
        break;
      case 'remove':
        $modifier = $modifier + $this->active_values;
        if (isset($modifier[$key])) {
          unset($modifier[$key]);
        }
        break;
      case 'active':
        $modifier = $modifier + $this->active_values;
        break;
    }
    $modifier = searchlight_environment_pack($modifier);
    if (!empty($modifier)) {
      $options['purl'] = array('provider' => 'searchlight', 'id' => $modifier);
    }
    else {
      $options['purl'] = array('remove' => array('searchlight'));
    }

    $exclude = array('q', 'page');
    $method = variable_get('purl_method_searchlight', 'path');
    $key = variable_get('purl_method_searchlight_key', FALSE);
    if ($method === 'querystring') {
      // If we've been asked to retain all facets, we need to exclude all
      // query strings **except** the facet query string.
      if ($op === 'active') {
        $exclude = drupal_map_assoc(array_keys($_GET));
        if (isset($exclude[$key])) {
          unset($exclude[$key]);
        }
      }
      // If there is no modifier we want to exclude the facet query string
      // (forces query string to be dropped).
      else if (empty($modifier)) {
        $exclude[] = $key;
      }
    }
    $options['query'] = drupal_query_string_encode($_GET, $exclude);

    return $options;
  }
}
