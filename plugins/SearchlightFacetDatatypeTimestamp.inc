<?php

class SearchlightFacetDatatypeTimestamp extends SearchlightFacet {
  /**
   * Override of query().
   */
  function query(&$query, $arg) {
    $granularity = !empty($this->field['granularity']) ? $this->field['granularity'] : 'month';
    $range = $this->query->backend->dateRange($arg, $granularity);

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
}
