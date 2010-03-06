<?php

class sphinx_views_handler_sort_search extends views_handler_sort {
  /**
   * Override of query().
   */
  function query() {
    $this->view->sphinx_views = isset($this->view->sphinx_views) ? $this->view->sphinx_views : array();
    $this->query->add_orderby(NULL, 'sphinx_weight', $this->options['order'], 'sphinx_weight');
    if (isset($this->query->fields['sphinx_weight'])) {
      unset($this->query->fields['sphinx_weight']);
    }
  }
}
