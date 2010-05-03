<?php

class SearchlightFacetSearchQuery extends SearchlightFacet {
  /**
   * Build facet and counts using search backend.
   */
  function build() {
    return array(
      $this->query->search_query => array(
        'id' => $this->query->search_query,
        'title' => $this->query->search_query,
      ),
    );
    return $items;
  }

  /**
   * Generate a label for this facet.
   */
  function label($delta, $id = NULL) {
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
  function render($delta, $id = NULL) {
    $rendered = array();
    if ($delta === 'active' && !empty($this->query->search_query)) {
      $item = array_shift($this->build());
      $item['link'] = l(t('remove'), $_GET['q'], searchlight_facet_manager()->getURLOptions('active'));
      $rendered[] = theme('searchlight_facet_active', $this->field, $item);
    }
    return $rendered;
  }
}
