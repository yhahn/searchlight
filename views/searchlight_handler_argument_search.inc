<?php

class searchlight_handler_argument_search extends views_handler_argument {
  /**
   * Override of pre_query().
   */
  function pre_query() {
    searchlight_views_init_query($this->view);
  }

  /**
   * Override of query().
   */
  function query() {
    $value = is_array($this->value) ? reset($this->value) : $this->value;
    $value = trim($value);
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
    if ($backend = searchlight_get_backend()) {
      $options += $backend->viewsOptionDefinition($this);
    }
    return $options;
  }

  /**
   * Override of options_form().
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    if ($backend = searchlight_get_backend()) {
      $backend->viewsOptionsForm($form, &$form_state, $this);
    }
  }
}
