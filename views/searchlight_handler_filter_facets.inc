<?php

class searchlight_handler_filter_facets extends views_handler_filter {
  /**
   * Override of query().
   */
  function query() {
    if (!empty($this->query->searchlight) && $environment = searchlight_environment_get()) {
      // Retrieve active facets. We will consider the presence of any active
      // facets to mean the search query is *not* empty.
      $active_facets = $environment->getFacets(TRUE);
      if (!empty($active_facets)) {
        $this->query->search_options['hideEmpty'] = FALSE;
        $environment->query($this->view, $this->query);
      }
    }
  }

  function can_expose() { return FALSE; }
}
