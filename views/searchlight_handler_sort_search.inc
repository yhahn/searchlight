<?php

class searchlight_handler_sort_search extends views_handler_sort {
  /**
   * Override of query().
   */
  function query() {
    // @TODO: I don't like this. Add to searchlight params, not to query object.
    $this->view->searchlight = isset($this->view->searchlight) ? $this->view->searchlight : array();
    $this->query->add_orderby(NULL, 'searchlight_weight', $this->options['order'], 'searchlight_weight');
    if (isset($this->query->fields['searchlight_weight'])) {
      unset($this->query->fields['searchlight_weight']);
    }
  }
}
