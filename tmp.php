<?php
  /**
   * Execute the search query and provide results for the given view.
   */
  function viewsExecute(&$view) {
    $this->views["{$view->name}:{$view->current_display}"] = $view;

    // Skip Views' query execution - we will do it ourselves.
    $view->executed = TRUE;

    // Retrieve active facets. We will consider the presence of any active
    // facets to mean the search query is *not* empty.
    $view->searchlight['active_facets'] = searchlight_facets()->activeFacets($view->searchlight['datasource']);

    // If the search query is empty and the handler has the hideEmpty option
    // enabled, abort right here.
    $view->searchlight['query'] = trim($view->searchlight['query']);
    if (
      $view->searchlight['options']['hideEmpty'] &&
      empty($view->searchlight['query']) &&
      empty($view->searchlight['active_facets'])
    ) {
      return;
    }

    // Give the backend a chance to init.
    $this->queryInit($view->searchlight);

    // Views query token replacements.
    $replacements = module_invoke_all('views_query_substitutions', $view);

    // Set filters.
    foreach ($view->query->where as $group => $where) {
      // Copy where args and do token replacements.
      // These will be passed by reference to set_filter() so it can eat
      // arguments progressively from clauses.
      $where_args = !empty($where['args']) ? $where['args'] : array();
      if (is_array($where_args)) {
        foreach ($where_args as $id => $arg) {
          $where_args[$id] = str_replace(array_keys($replacements), $replacements, $arg);
        }
      }
      foreach ($where['clauses'] as $key => $clause) {
        // Query Views token replacements.
        $clause = str_replace(array_keys($replacements), $replacements, $clause);
        if ($translated = $this->translateWhere($view->searchlight, $clause, $where_args)) {
          $this->setFilter($view->searchlight, $translated['field'], $translated['operator'], $translated['args']);
        }
      }
    }

    // Set filters for active facets.
    foreach ($view->searchlight['active_facets'] as $name => $arg) {
      $field = $view->searchlight['datasource']['fields'][$name];
      switch ($field['datatype']) {
        case 'timestamp':
          $granularity = !empty($field['granularity']) ? $field['granularity'] : 'month';
          $range = $this->dateRange($arg, $granularity);
          $this->setFilter($view->searchlight, $name, '>', array($range['from']));
          $this->setFilter($view->searchlight, $name, '<', array($range['to']));
          break;
        default:
          $this->setFilter($view->searchlight, $name, '=', array($arg));
          break;
      }
    }

    // If base table is node, add node access. Since node_access can require
    // complex conditions (in particular, OR), delegate to backend.
    $this->setNodeAccess($view->searchlight);

    // Set sorts.
    foreach ($view->query->orderby as $orderby) {
      if ($translated = $this->translateOrderby($view->searchlight, $orderby, $view->query->fields)) {
        $this->setSort($view->searchlight, $translated['field'], $translated['direction']);
      }
    }

    // Set pager.
    $pagers = isset($_GET['page']) ? explode(',', $_GET['page']) : array();
    $offset = $items_per_page = 0;
    if (!empty($view->pager['items_per_page'])) {
      $offset = ($pagers[$view->pager['element']] * $view->pager['items_per_page']) + $view->pager['offset'];
      $items_per_page = $view->pager['items_per_page'];
    }
    $this->setPager($view->searchlight, $offset, $items_per_page);

    // Execute the query.
    $result = $this->queryExecute($view->searchlight);
    if ($result) {
      $view->total_rows = $result['total_rows']; // - $this->view->pager['offset'];

      // Set pager information
      if (!empty($view->pager['use_pager'])) {
        // dump information about what we already know into the globals
        global $pager_page_array, $pager_total, $pager_total_items;
        // total rows in query
        $pager_total_items[$view->pager['element']] = $view->total_rows;
        // total pages
        $pager_total[$view->pager['element']] = ceil($pager_total_items[$view->pager['element']] / $view->pager['items_per_page']);
        // What page was requested:
        $pager_page_array = isset($_GET['page']) ? explode(',', $_GET['page']) : array();

        // If the requested page was within range. $view->pager['current_page']
        // defaults to 0 so we don't need to set it in an out-of-range condition.
        if (!empty($pager_page_array[$view->pager['element']])) {
          $page = intval($pager_page_array[$view->pager['element']]);
          if ($page > 0 && $page < $pager_total[$view->pager['element']]) {
            $view->pager['current_page'] = $page;
          }
        }
        $pager_page_array[$view->pager['element']] = $view->pager['current_page'];
      }

      // Clear out normal field, sort, where portions of the query that
      // have been taken care of by the backend.
      $view->query->orderby = array();
      $view->query->where = array();

      // @TODO: do this with placeholders, args.
      $ids = implode(',', $result['result']);
      $view->query->add_where(0, "{$view->base_table}.{$view->base_field} IN ({$ids})");

      // Build query, args
      $main = $view->query->query();
      $args = $view->query->get_where_args();

      // Replace tokens in the query, args.
      $main = str_replace(array_keys($replacements), $replacements, $main);
      if (is_array($args)) {
        foreach ($args as $id => $arg) {
          $args[$id] = str_replace(array_keys($replacements), $replacements, $arg);
        }
      }

      // Execute query and build result set.
      $dataset = array_fill_keys($result['result'], FALSE);
      $views_result = db_query($main, $args);
      while ($row = db_fetch_object($views_result)) {
        if (isset($dataset[$row->{$view->base_field}])) {
          $dataset[$row->{$view->base_field}] = $row;
        }
      }
      $dataset = array_filter($dataset);
      $dataset = array_values($dataset);
      $view->result = $dataset;
    }
  }