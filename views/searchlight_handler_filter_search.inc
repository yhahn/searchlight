<?php

class sphinx_views_handler_filter_search extends views_handler_filter {
  /**
   * Override of query().
   */
  function query() {
    $this->view->sphinx_views = isset($this->view->sphinx_views) ? $this->view->sphinx_views : array();

    $value = trim(current($this->value));
    if (!empty($value) || !$this->options['sphinx']['hideEmpty']) {
      $this->view->sphinx_views['query'] = $value;
      $this->view->sphinx_views['matchMode'] = $this->options['sphinx']['matchMode'];
    }
    else {
      $this->view->sphinx_views['abort'] = TRUE;
    }
  }

  /**
   * Override of options_form().
   */
  function options_form(&$form, &$form_state) {
    module_load_include('php', 'sphinx_views', 'sphinxapi');
    parent::options_form($form, $form_state);
    $form['sphinx'] = array(
      '#title' => t('Sphinx search'),
      '#tree' => TRUE,
      '#type' => 'fieldset',
    );
    $form['sphinx']['matchMode'] = array(
      '#title' => t('Search mode'),
      '#type' => 'select',
      '#options' => array(
        SPH_MATCH_ALL => t('Match all query words'),
        SPH_MATCH_ANY => t('Match any of the query words'),
        SPH_MATCH_PHRASE => t('Match the exact phrase'),
        // SPH_MATCH_BOOLEAN => t('Match query as a boolean syntax'),
        // SPH_MATCH_EXTENDED2 => t('Match query as a boolean syntax'),
      ),
      '#default_value' => $this->options['sphinx']['matchMode'],
    );
    $form['sphinx']['hideEmpty'] = array(
      '#title' => t('Hide all results when empty'),
      '#type' => 'checkbox',
      '#default_value' => $this->options['sphinx']['hideEmpty'],
    );
  }

  /**
   * Override of option_definition().
   */
  function option_definition() {
    module_load_include('php', 'sphinx_views', 'sphinxapi');
    $options = parent::option_definition();
    $options['sphinx'] = array(
      'default' => array(
        'matchMode' => SPH_MATCH_ALL,
        'hideEmpty' => TRUE,
      ),
    );
    return $options;
  }

  /**
   * Provide a textfield for search query.
   */
  function value_form(&$form, &$form_state) {
    $form['value'] = array(
      '#type' => 'textfield',
      '#title' => t('Value'),
      '#size' => 30,
      '#default_value' => $this->value,
    );
    if (!empty($form_state['exposed']) && !isset($form_state['input'][$identifier])) {
      $form_state['input'][$identifier] = $this->value;
    }
  }
}
