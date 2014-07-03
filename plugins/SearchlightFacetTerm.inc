<?php

/**
 * Facet plugin class for taxonomy terms.
 */
class SearchlightFacetTerm extends SearchlightFacet {
  /**
   * Override viewRenderItems to use a simple db_select().
   */
  function viewRenderItems($items) {
    $result = db_select('taxonomy_term_data')
      ->fields('taxonomy_term_data', array('name', 'tid'))
      ->condition('tid', array_keys($items))
      ->execute();
    foreach ($result as $term) {
      if (isset($items[$term->tid])) {
        $items[$term->tid]['title'] = $term->name;
      }
    }
    return $items;
  }
}
