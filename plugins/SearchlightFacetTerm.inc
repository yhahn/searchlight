<?php

class SearchlightFacetTerm extends SearchlightFacet {
  /**
   * Override of construct().
   * Do additional setup and provide information about the current active facet
   * term, vocab and hierarchy.
   */
  function construct($environment, $field, $value, $options) {
    parent::construct($environment, $field, $value, $options);

    $this->term = taxonomy_get_term($this->value);

    // Attempt to parse the vocab from the field name.
    // @TODO: Refactor multivalue handling to make this more palatable.
    $vid = trim(array_pop(explode('_', $this->name)), 'v');
    $this->vocab = taxonomy_vocabulary_load($vid);
    if (!empty($this->vocab->hierarchy)) {
      $this->tree = taxonomy_get_tree($this->vocab->vid);
    }

    $this->children = array();
    $this->parents = array();
    if (!empty($this->value) && $this->term) {
      foreach (taxonomy_get_children($this->term->tid) as $term) {
        $this->children[] = $term->tid;
      }
      foreach (taxonomy_get_parents($this->term->tid) as $term) {
        $this->parents[] = $term->tid;
      }
    }
  }

  /**
   * Override of query().
   * Handle hierarchical taxonomies.
   */
  function query(&$query) {
    // Filter the query if there is an active facet value.
    if (isset($this->value)) {
      $query->search_filter[] = array(
        'field' => $this->name,
        'operator' => 'IN',
        'args' => array_merge(array($this->value), $this->children),
      );
    }

    // Add this facet to be built by the backend.
    // An extremely high limit is used so that we can process returned terms
    // properly Drupalside.
    // @TODO: This is probably a terrible idea.
    if (!empty($this->vocab->hierarchy)) {
      $limit = 1000;
    }
    else {
      $limit = isset($this->options['items']) ? $this->options['items'] : 5;
    }
    $query->add_search_facet($this->name, $limit);
  }


  /**
   * Override of render().
   * Most of the complexity here is for smart handling of
   * hierarchical taxonomies.
   */
  function render($query, $delta) {
    $rendered = array();
    if ($this->viewInit($query)) {
      switch ($delta) {
        case 'active':
          if (isset($this->value)) {
            // Add each parent term and the term itself.
            // Each non-root term sets the facet to its direct parent term
            // rather than removing the facet completely.
            $trail = $this->parents;
            $trail[] = $this->value;
            $last = NULL;
            foreach ($trail as $num => $id) {
              if (isset($last)) {
                $options = $this->environment->getURLOptions('add', $this->name, $last);
              }
              else {
                $options = $this->environment->getURLOptions('remove', $this->name, $id);
              }
              $item = array('id' => $id, 'title' => $this->getName($id), 'link' => l(t('remove'), $_GET['q'], $options));
              $rendered[$id] = array(
                'data' => theme('searchlight_facet_active', $this->field, $item),
                'class' => 'searchlight-facet-depth-' . $num
              );
              $last = $id;
            }
            if ($this->vocab->hierarchy && $this->term) {
              $raw = $query->get_search_facet($this->name);
              $terms = taxonomy_get_children($this->term->tid, $this->vocab->vid);
              foreach ($terms as $term) {
                $count = $this->getDeepCount($term->tid, $raw);
                if ($count) {
                  $options = $this->environment->getURLOptions('add', $this->name, $term->tid);
                  $item = array('id' => $term->tid, 'title' => $term->name, 'count' => $count, 'link' => l($term->name, $_GET['q'], $options));
                  $rendered[$term->tid] = array('class' => 'searchlight-facet-depth-' . count($trail), 'data' => theme('searchlight_facet_link', $this->field, $item));
                }
              }
            }
          }
          break;
        case 'facets':
          if ($this->vocab->hierarchy && empty($this->term)) {
            $raw = $query->get_search_facet($this->name);
            $terms = taxonomy_get_children(0, $this->vocab->vid);
            foreach ($terms as $term) {
              $count = $this->getDeepCount($term->tid, $raw);
              if ($count) {
                $options = $this->environment->getURLOptions('add', $this->name, $term->tid);
                $item = array('id' => $term->tid, 'title' => $term->name, 'count' => $count, 'link' => l($term->name, $_GET['q'], $options));
                $rendered[$term->tid] = theme('searchlight_facet_link', $this->field, $item);
              }
            }
          }
          else if (!isset($this->value)) {
            foreach ($query->get_search_facet($this->name) as $item) {
              $options = $this->environment->getURLOptions('add', $this->name, $item['id']);
              $item['title'] = $this->getName($item['id']);
              $item['link'] = l($item['title'], $_GET['q'], $options);
              $rendered[$item['id']] = theme('searchlight_facet_link', $this->field, $item);
            }
          }
          break;
      }
    }
    return $rendered;
  }

  /**
   * Simple renderer for term names.
   */
  function getName($tid) {
    if ($term = taxonomy_get_term($tid)) {
      return $term->name;
    }
    return NULL;
  }

  /**
   * Retrieve the full sum count (including all children) for a term.
   */
  function getDeepCount($tid, $raw) {
    $count = isset($raw[$tid]['count']) ? $raw[$tid]['count'] : 0;
    $children = taxonomy_get_children($tid);
    $children = array_intersect_key($raw, $children);
    if (!empty($children)) {
      foreach (array_keys($children) as $tid) {
        $count = $count + $this->getDeepCount($tid, $raw);
      }
    }
    return $count;
  }
}
