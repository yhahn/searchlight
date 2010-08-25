<?php

/**
 * Facet plugin class for language.
 */
class SearchlightFacetLanguage extends SearchlightFacet {
  /**
   * Override of query().
   * Merges null value filtering with the site default language.
   */
  function query(&$query) {
    // Filter the query if there is an active facet value.
    if (isset($this->value)) {
      $default = language_default('language');
      if (empty($this->value) || $this->value === $default) {
        $query->search_filter[] = array(
          'field' => $this->name,
          'operator' => 'IN',
          'args' => array('', $default),
        );
      }
      else {
        $query->search_filter[] = array(
          'field' => $this->name,
          'operator' => '=',
          'args' => array($this->value),
        );
      }
    }

    // Add this facet to be built by the backend.
    $limit = isset($this->options['items']) ? $this->options['items'] : 5;
    $query->add_search_facet($this->name, $limit);
  }

  /**
   * Override of render().
   * Merges null value facet with the site default language facet.
   */
  function render($query, $delta) {
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
    if (!empty($items) && $this->viewInit($query)) {
      $default = language_default('language');
      if (isset($items[$default], $items[''])) {
        $items[$default]['count'] = $items[$default]['count'] + $items['']['count'];
        unset($items['']);
      }
      return $this->viewRenderItems($items);
    }
    return array();
  }
}
