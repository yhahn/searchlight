<?php

/**
 * Facet plugin class for taxonomy terms.
 */
class SearchlightFacetTerm extends SearchlightFacet {
  /**
   * Override of construct().
   * Do additional setup and provide information about the current active facet
   * term, vocab and hierarchy.
   */
  function construct($environment, $field, $value, $options) {
    parent::construct($environment, $field, $value, $options);

    $this->term = taxonomy_term_load($this->value);

    // Attempt to parse the vocab from the field name.
    $identifier = trim(array_pop(explode('taxonomy_term_data_tid_', $this->name)));
    if (is_numeric($identifier)) {
      $vid = $identifier;
    }
    else {
      $vid = db_query("SELECT vid FROM {taxonomy_vocabulary} WHERE module = :module LIMIT 1", array(':module' => $identifier))->fetchField();
    }
    $this->vocab = taxonomy_vocabulary_load($vid);
    if (!empty($this->vocab->hierarchy)) {
      $this->tree = array();
      foreach (taxonomy_get_tree($this->vocab->vid) as $term) {
        $this->tree[$term->tid] = $term;
      }
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
   * Override of theme().
   */
  function theme($item, $delta) {
    if ($this->vocab->hierarchy && $delta === 'active') {
      if (isset($this->tree[$this->value], $this->tree[$item['id']])) {
        $term_item = $this->tree[$item['id']];
        $term_current = $this->tree[$this->value];

        // Add a class specifying the item's depth.
        $item['class'] = "depth-{$term_item->depth}";

        // Compare this item's depth vs. current value depth.
        // If the item is deeper than the current depth it should be an option
        // for further faceting drill-down.
        if ($term_item->depth > $term_current->depth) {
          // TODO Please change this theme call to use an associative array for the $variables parameter.
          // TODO Please change this theme call to use an associative array for the $variables parameter.
          return parent::theme($item, 'facets');
        }
        // Otherwise, broaden the search by either linking to a parent term
        // facet or (root terms) removing the term facets alltogether.
        else {
          if ($parent = reset($term_item->parents)) {
            $path = $this->environment->getURLPath();
            $options = $this->environment->getURLOptions('add', $this->name, $parent);
            $item['link'] = l(t('remove'), $path, $options);
          }
          else {
            $path = $this->environment->getURLPath();
            $options = $this->environment->getURLOptions('remove', $this->name, $term_item->tid);
            $item['link'] = l(t('remove'), $path, $options);
          }
          return theme('searchlight_facet_active', $this->field, $item);
        }
      }
      return '';
    }
    // TODO Please change this theme call to use an associative array for the $variables parameter.
    // TODO Please change this theme call to use an associative array for the $variables parameter.
    return parent::theme($item, $delta);
  }


  /**
   * Override of render().
   * Most of the complexity here is for smart handling of
   * hierarchical taxonomies.
   */
  function render($query, $delta) {
    if ($this->viewInit($query)) {
      switch ($delta) {
        case 'active':
          if (isset($this->value)) {
            $items = array();
            // Add terms from current value all the way to root term.
            $trail = $this->parents;
            $trail[] = $this->value;
            foreach ($trail as $num => $id) {
              $items[] = array(
                'id' => $id,
                'title' => $this->getName($id),
              );
            }
            // Add direct children terms and get their counts.
            if ($this->vocab->hierarchy && $this->term) {
              $raw = $query->get_search_facet($this->name);
              $terms = taxonomy_get_children($this->term->tid, $this->vocab->vid);
              foreach ($terms as $term) {
                $count = $this->getDeepCount($term->tid, $raw);
                if ($count) {
                  $items[] = array(
                    'id' => $term->tid,
                    'title' => $term->name,
                    'count' => $count,
                  );
                }
              }
            }
            return $items;
          }
          break;
        case 'facets':
          $items = array();
          // If hierarchical, only show root terms and calculate their "deep"
          // count values.
          if ($this->vocab->hierarchy && empty($this->term)) {
            $raw = $query->get_search_facet($this->name);
            $terms = taxonomy_get_children(0, $this->vocab->vid);
            foreach ($terms as $term) {
              $count = $this->getDeepCount($term->tid, $raw);
              if ($count) {
                $items[] = array(
                  'id' => $term->tid,
                  'title' => $term->name,
                  'count' => $count, /*'link' => l($term->name, $_GET['q'], $options)*/
                );
              }
            }
          }
          // If non-hierarchical, simple flat list of terms.
          else if (!isset($this->value)) {
            foreach ($query->get_search_facet($this->name) as $item) {
              $item['title'] = $this->getName($item['id']);
              $items[] = $item;
            }
          }
          return $items;
          break;
      }
    }
    return array();
  }

  /**
   * Simple renderer for term names.
   */
  function getName($tid) {
    if ($term = taxonomy_term_load($tid)) {
      return $term->name;
    }
    return NULL;
  }

  /**
   * Retrieve the full sum count (including all children) for a term.
   * @TODO: This is wrong! If a node is tagged with both the parent term and
   * a child term (or more) it will be counted multiple times. Fix this.
   */
  function getDeepCount($tid, $raw) {
    $count = isset($raw[$tid]['count']) ? $raw[$tid]['count'] : 0;
    if ($this->options['count_method'] === 'deep') {
      $children = taxonomy_get_children($tid);
      $children = array_intersect_key($raw, $children);
      if (!empty($children)) {
        foreach (array_keys($children) as $tid) {
          $count = $count + $this->getDeepCount($tid, $raw);
        }
      }
    }
    return $count;
  }

  /**
   * Provide default values for options.
   */
  function optionsDefault() {
    $options = parent::optionsDefault();
    $options['deep_count'] = 'yes';
    return $options;
  }

  /**
   * Provide an options form to be exposed in the Environment editor.
   */
  function extendedForm(&$form, $form_state) {
    parent::extendedForm($form, $form_state);
    $form['count_method'] = array(
      '#title' => t('Count mode'),
      '#type' => 'select',
      '#options' => array(
        'shallow' => t('Shallow'),
        'deep' => t('Deep'),
      ),
      '#default_value' => $this->options['count_method'],
    );
  }
}
