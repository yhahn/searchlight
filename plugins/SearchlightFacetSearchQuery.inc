<?php

class SearchlightFacetSearchQuery extends SearchlightFacet {
  /**
   * Override of query().
   */
  function query(&$query) { }

  /**
   * Build facet and counts using search backend.
   */
  function build($query) {
    return array(
      $query->search_query => array(
        'id' => $query->search_query,
        'title' => $query->search_query,
      ),
    );
    return $items;
  }

  /**
   * Generate a label for this facet.
   */
  function label($delta) {
    if (!empty($this->options['label'])) {
      return $this->options['label'];
    }
    return t('Search');
  }

  /**
   * Override of render().
   * The SearchQuery is only rendered active and when a search query is present
   * on the current page query.
   */
  function render($query, $delta) {
    $rendered = array();
    if ($delta === 'active' && !empty($query->search_query)) {
      $item = array_shift($this->build($query));
      $item['link'] = l(t('remove'), $_GET['q'], $this->environment->getURLOptions('active'));
      $rendered[] = theme('searchlight_facet_active', $this->field, $item);
    }
    return $rendered;
  }
}
