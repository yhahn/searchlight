<?php

class searchlight_handler_filter_facets extends views_handler_filter {
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
    if (!empty($this->query->searchlight) && $environment = searchlight_environment_active(NULL, $this->view->name, $this->view->current_display)) {
      if ($environment->initView($this->view)) {
        $environment->query($this->query);
      }
    }
  }

  function can_expose() { return FALSE; }
}
