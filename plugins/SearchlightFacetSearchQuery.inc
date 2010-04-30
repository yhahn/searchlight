<?php

class SearchlightFacetSearchQuery extends SearchlightFacet {
  /**
   * Build facet and counts using search backend.
   */
  function build($query, $name, $limit) {
    $items = array();
    $items = array(
      $query->search_query => array(
        'id' => $query->search_query,
        'title' => $query->search_query,
      ),
    );
    return $items;
  }

  /**
   * Render all items of a facet.
   */
  function render($query, $name, $limit) {
    if (!empty($query->search_query)) {
      $items = $this->build($query, $name, $limit);
      $item = reset($items);
      $item['link'] = l(t('remove'), $_GET['q'], searchlight_facet_manager()->getURLOptions('active'));
      return array(
        'label' => t('Search'),
        'items' => array(theme('searchlight_facet_active', array(), $item)),
      );
    }
    return fALSE;
  }
}
