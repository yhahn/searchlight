<?php

class searchlight_handler_filter_facets extends views_handler_filter {
  /**
   * Override of query().
   */
  function query() {
    if (!empty($this->query->searchlight) && $environment = searchlight_environment_get()) {
      if ($environment->initView($this->view)) {
        $environment->query($this->query);
      }
    }
  }

  function can_expose() { return FALSE; }
}
