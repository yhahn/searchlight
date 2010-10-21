<?php

class searchlight_handler_sort_search extends views_handler_sort {
  /**
   * Override of query().
   */
  function query() {
    $this->query->add_orderby(NULL, 'searchlight_weight', $this->options['order'], 'searchlight_weight');
    if (isset($this->query->fields['searchlight_weight'])) {
      unset($this->query->fields['searchlight_weight']);

      foreach ($this->query->orderby as $key => $info) {
        if ($info['field'] == 'searchlight_weight') {
          unset($this->query->orderby[$key]);
        }
      }
    }
  }
}
