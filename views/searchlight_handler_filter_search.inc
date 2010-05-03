<?php

class searchlight_handler_filter_search extends views_handler_filter {
  /**
   * Override of query().
   */
  function query() {
    $value = trim(current($this->value));
    $this->query->set_search_options($this->options);
    $this->query->set_search_query($value);
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

  /**
   * Provide a textfield for search query.
   */
  function value_form(&$form, &$form_state) {
    if ($backend = searchlight_get_backend()) {
      $backend->viewsValueForm($form, &$form_state, $this);
    }
  }
}
