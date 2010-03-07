<?php

/**
 * Abstract class, defines interface for swappable search backends.
 */
class SearchlightBackend {
  var $settings;
  var $views;

  /**
   * Constructor. Unpack backend variable & populate default settings.
   */
  function __construct($id) {
    $this->id = $id;
    $this->settings = variable_get("searchlight_backend_{$this->id}", array()) + $this->settingsDefault();
  }

  /**
   * Array of default settings for this search backend.
   */
  function settingsDefault() {
    return array();
  }

  /**
   * System settings form for this search backend.
   */
  function settingsForm($values) {
    return array();
  }

  /**
   * Backend callback for filter, argument views handler
   * $handler->option_definition() method.
   */
  function viewsOptionDefinition(&$handler) {
    return array(
      'hideEmpty' => array('default' => TRUE),
    );
  }

  /**
   * Backend callback for filter, argument views handler
   * $handler->options_form() method.
   */
  function viewsOptionsForm(&$form, &$form_state, &$handler) {
    $form['hideEmpty'] = array(
      '#title' => t('Hide all results when empty'),
      '#type' => 'checkbox',
      '#default_value' => $handler->options['hideEmpty'],
    );
  }

  /**
   * Backend callback for filter handler $handler->value_form() method.
   */
  function viewsValueForm(&$form, &$form_state, &$handler) {
    $form['value'] = array(
      '#type' => 'textfield',
      '#title' => t('Value'),
      '#size' => 30,
      '#default_value' => $this->value,
    );
    if (!empty($form_state['exposed']) && !isset($form_state['input'][$identifier])) {
      $form_state['input'][$identifier] = $this->value;
    }
  }

  /**
   * Execute the search query and provide results for the given view.
   */
  function viewsExecute(&$view) {
    $this->views["{$view->name}:{$view->current_display}"] = $view;

    // Skip Views' query execution - we will do it ourselves.
    $view->executed = TRUE;

    // If the search query is empty and the handler has the hideEmpty option
    // enabled, abort right here.
    $view->searchlight['query'] = trim($view->searchlight['query']);
    if (empty($view->searchlight['query']) && $view->searchlight['handler']->options['hideEmpty']) {
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

  /**
   * Let the backend init its query params/object.
   */
  function queryInit(&$searchlight) {
  }

  /**
   * Let the backend execute its query.
   *
   * @return An array with the following:
   *
   * 'result': An array of primary IDs for the given base table, in the
   *   preferred order of the resultset.
   * 'total_rows': An integer value for the total number of rows in the
   *   full resultset. Used by Views for paging.
   */
  function queryExecute(&$searchlight) {
    return array(
      'result' => array(),
      'total_rows' => 0,
    );
  }

  /**
   * Translate a Views query object WHERE clause into filter params for the
   * search backend.
   */
  function translateWhere(&$searchlight, $clause, &$where_args) {
    $split = preg_split('/[ ]([<>=!]*)|(IN)|(NOT IN)/i', trim($clause), NULL, PREG_SPLIT_NO_EMPTY | PREG_SPLIT_DELIM_CAPTURE);
    if (count($split) >= 2) {
      $field = explode('.', trim(array_shift($split)));
      $operator = strtoupper(trim(array_shift($split)));
      if (count($field) === 2) {
        $table = $field[0];
        $field = $field[1];
        foreach ($searchlight['datasource']['fields'] as $name => $info) {
          if ($info['table'] === $table && $info['field'] === $field) {
            // Get the arguments for this where clause.
            $args = array();
            $argnum = substr_count($clause, '%');
            if (count($where_args) >= $argnum) {
              while ($argnum > 0) {
                $args[] = array_shift($where_args);
                $argnum--;
              }
            }
            return array('field' => $name, 'operator' => $operator, 'args' => $args);
          }
        }
      }
    }
    return FALSE;
  }

  /**
   * Translate a Views query object ORDER BY clause into sort params for the
   * search backend.
   */
  function translateOrderby(&$searchlight, $orderby, $query_fields) {
    $split = explode(' ', $orderby);
    if (count($split) <= 2) {
      $field = $split[0];
      $direction = isset($split[1]) ? strtoupper($split[1]) : 'ASC';
      if ($field === 'searchlight_weight') {
        return array('field' => 'searchlight_weight', 'direction' => $direction);
      }
      // Orderby field is *not* aliased. Retrieve table, field.
      if (strpos($field, '.') !== FALSE) {
        list($table, $field) = explode('.', $field);
      }
      // Field is aliased. Use query fields to retrieve actual table, field.
      else {
        if (isset($query_fields[$field])) {
          $table = $query_fields[$field]['table'];
          $field = $query_fields[$field]['field'];
        }
      }
      if (isset($table, $field)) {
        foreach ($searchlight['datasource']['fields'] as $name => $info) {
          if ($info['table'] === $table && $info['field'] === $field) {
            return array('field' => $name, 'direction' => $direction);
          }
        }
      }
    }
    return FALSE;
  }

  /**
   * Set a filter parameter for the search backend.
   */
  function setFilter(&$searchlight, $field, $operator, $args) {
  }

  /**
   * Set a sort parameter for the search backend.
   */
  function setSort(&$searchlight, $field, $direction) {
  }

  /**
   * Set a pager/limit parameter for the search backend.
   */
  function setPager(&$searchlight, $offset, $items_per_page) {
  }
}
