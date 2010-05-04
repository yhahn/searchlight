<?php

class SearchlightFacetDatatypeTimestamp extends SearchlightFacet {
  /**
   * Override of query().
   */
  function query(&$query) {
    if (isset($this->value)) {
      $granularity = !empty($this->field['granularity']) ? $this->field['granularity'] : 'month';
      $range = $query->backend->dateRange($this->value, $granularity);

      $query->search_filter[] = array(
        'field' => $this->name,
        'operator' => '>',
        'args' => array($range['from']),
      );
      $query->search_filter[] = array(
        'field' => $this->name,
        'operator' => '<',
        'args' => array($range['to']),
      );
    }

    // Add this facet to be built by the backend.
    $limit = isset($this->options['items']) ? $this->options['items'] : 5;
    $query->add_search_facet($this->name, $limit);
  }
}
