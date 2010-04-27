<?php

class searchlight_handler_argument_search extends views_handler_argument {
  /**
   * Override of query().
   */
  function query() {
    $value = trim($this->argument);
    if ($this->argument_validated && !empty($value)) {
      $this->query->set_search_options($this->options);
      $this->query->set_search_query($value);
    }
  }

  /**
   * Override of option_definition().
   */
  function option_definition() {
    $options = parent::option_definition();
    if ($backend = searchlight_default_backend()) {
      $options += $backend->viewsOptionDefinition($this);
    }
    return $options;
  }

  /**
   * Override of options_form().
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    if ($backend = searchlight_default_backend()) {
      $backend->viewsOptionsForm($form, &$form_state, $this);
    }
  }
}
