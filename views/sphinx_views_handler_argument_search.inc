<?php

class sphinx_views_handler_argument_search extends views_handler_argument {
  /**
   * Override of query().
   */
  function query() {
    $value = trim($this->argument);
    if ($this->argument_validated && !empty($value)) {
      $this->view->sphinx_views = isset($this->view->sphinx_views) ? $this->view->sphinx_views : array();
      $this->view->sphinx_views['query'] = $value;
      $this->view->sphinx_views['matchMode'] = $this->options['sphinx']['matchMode'];
    }
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
      ),
    );
    return $options;
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
  }
}
