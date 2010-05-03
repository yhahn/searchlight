<?php

class SearchlightEnvironment {
  // Storage variables: saved to DB or code when an environment is saved.
  var $name;
  var $datasource;
  var $facets;
  var $options;

  // Working variables: populated at various init and other stages.
  var $enabled;
  var $active_values;

  // Class objects.
  var $plugins;
  var $query;
  var $view;

  function construct() {
    $this->name = isset($this->name) ? $this->name : '';
    $this->datasource = isset($this->datasource) ? $this->datasource : '';
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
   * Initialize facet plugins for this environment. Requires the current page's
   * view and query object to be passed.
   */
  function initPlugins($view = NULL, $query = NULL, $reset = FALSE) {
    if (empty($this->plugins) || $reset) {
      $this->view = NULL;
      $this->query = NULL;
      $this->plugins = array();

      $datasource = searchlight_datasource_load($this->datasource);

      // If no view, query passed, attempt to use current page view.
      if (!isset($view, $query)) {
        $display = views_get_page_view();
        if ($display && !empty($display->view->query->searchlight)) {
          $view = $display->view;
          $query = $display->view->query;
        }
      }

      // Init plugins.
      if ($view && $query && $datasource) {
        $this->view = $view;
        $this->query = drupal_clone($query);
        foreach ($this->getFacets() as $name => $facet) {
          $plugin = searchlight_get_facet($datasource, $name);
          $plugin->construct($this, $datasource->fields[$name], $this->getValue($name), $this->facets[$name]);
          $this->plugins[$name] = $plugin;
        }
      }
    }
  }

  /**
   * Allow the environment to initialize plugins and allow them to modify the
   * Views query.
   */
  function query($view, $query) {
    $this->initPlugins($view, $query);
    foreach ($this->plugins as $name => $plugin) {
      $plugin->query($query);
    }
  }

  /**
   * Generate a set of renderable facet items.
   */
  function render($delta = 'active') {
    $this->initPlugins();
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
    return array();
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
      case 'edit-datasource-update':
        $this->datasource = $form_state['values']['datasource']['datasource'];
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
    else if ($datasource = searchlight_datasource_load($this->datasource)) {
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
