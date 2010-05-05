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

  /**
   * Render all items of a facet.
   */
  function render($query, $delta) {
    $rendered = array();
    switch ($delta) {
      case 'active':
        if (isset($this->value)) {
          $items = array($this->value => array('id' => $this->value));
        }
        break;
      case 'facets':
        if (!isset($this->value)) {
          $items = $query->get_search_facet($this->name);
        }
        break;
    }

    if (!empty($items)) {
      $op = ($delta === 'active' ? 'remove' : 'add');
      $granularity = !empty($this->field['granularity']) ? $this->field['granularity'] : 'month';
      foreach ($items as $id => $item) {
        switch ($granularity) {
          case 'year':
            $format = 'Y';
            break;
          case 'month':
            $format = 'F, Y';
            break;
          case 'week':
          case 'day':
            $format = 'F j, Y';
            break;
        }
        $item['title'] = format_date(strtotime($item['id']), 'custom', $format, 0);
        $title = $op === 'remove' ? t('remove') : $item['title'];
        $options = $this->environment->getURLOptions($op, $this->name, $id);
        $item['link'] = l($title, $_GET['q'], $options);
        $rendered[$id] = $op === 'remove' ?
          theme('searchlight_facet_active', $this->field, $item) :
          theme('searchlight_facet_link', $this->field, $item);
      }
    }
    return $rendered;
  }


}
