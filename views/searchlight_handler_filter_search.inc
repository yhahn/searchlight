<?php

class searchlight_handler_filter_search extends views_handler_filter {
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

    // There is a search query.
    if (!empty($value)) {
      $this->query->set_search_buildmode('search');
    }
    // There is no search query.
    else {
      // If we are replacing a core search filter the 'operator' option may be
      // set. Use this to determine buildmode.
      if (!empty($this->options['operator'])) {
        if ($this->options['operator'] === 'required') {
          $this->query->set_search_buildmode('empty');
        }
      }
      // Check the hideEmpty option and check whether to blank the resultset.
      elseif ($this->options['hideEmpty']) {
        $this->query->set_search_buildmode('empty');
      }
    }

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
      $backend->viewsOptionsForm($form, $form_state, $this);
    }
  }

  /**
   * Provide a textfield for search query.
   */
  function value_form(&$form, &$form_state) {
    if ($backend = searchlight_get_backend()) {
      $backend->viewsValueForm($form, $form_state, $this);
    }
  }
}
