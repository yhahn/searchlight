<?php

class SearchlightEnvironment {
  var $name;
  var $datasource;
  var $enabled;
  var $facets;
  var $options;

  var $query;
  var $view;

  function construct() {
    $this->name = isset($this->name) ? $this->name : '';
    $this->datasource = isset($this->datasource) ? $this->datasource : '';
    $this->facets = !empty($this->facets) ? $this->facets : array();
    $this->options = !empty($this->options) ? $this->options : array();

    $this->query = NULL;
    $this->view = NULL;

    $this->options = $this->options + array(
      'active_label' => t('Active search'),
      'facets_label' => t('Search facets'),
    );
  }

  /**
   * Activate this environment such that it can build/render facet displays.
   * Detect the current page view (if any) and clone its query object.
   */
  function activate($reset = FALSE) {
    if (!isset($this->query) || $reset) {
      $display = views_get_page_view();
      if ($display && !empty($display->view->query->searchlight)) {
        $this->query = drupal_clone($display->view->query);
        $this->view = $display->view;
      }
      else {
        $this->query = NULL;
        $this->view = NULL;
      }
    }
    return $this;
  }

  /**
   * Generate a set of renderable facet items.
   */
  function render($delta = 'active') {
    $this->activate();
    $rendered = array();
    foreach ($this->getFacets() as $name => $facet) {
      if ($plugin = searchlight_get_facet($this->query->datasource, $name)) {
        $id = searchlight_facet_manager()->getActive($name);
        $plugin->construct(drupal_clone($this->query), $this->query->datasource->fields[$name], $this->facets[$name]);
        if ($render = $plugin->render($delta, $id)) {
          $rendered[$name] = array('label' => $plugin->label($delta, $id), 'items' => $render);
        }
      }
    }
    return $rendered;
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
  function getFacets() {
    if (!isset($this->enabled)) {
      $this->enabled = array();
      $this->activate();
      if (!empty($this->query)) {
        foreach ($this->facets as $name => $facet) {
          if (!empty($facet['enabled']) && $this->isValidFacet($name)) {
            // Set set #weight so element_sort can be used.
            $facet['#weight'] = isset($facet['weight']) ? $facet['weight'] : 0;
            $this->enabled[$name] = $facet;
          }
        }
      }
      uasort($this->enabled, 'element_sort');
    }
    return $this->enabled;
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
}
