<?php

class searchlight_plugin_query extends views_plugin_query_default {
  var $backend;
  var $client;
  var $datasource;
  var $datasource_id;
  var $search_buildmode;
  var $search_facets;
  var $search_filter;
  var $search_options;
  var $search_result;
  var $search_sort;
  var $search_query;
  var $searchlight;

  /**
   * Override of init().
   */
  function init($base_table = 'node', $base_field = 'nid', $options = array(), $backend) {
    parent::init($base_table, $base_field, $options);

    // Init the fields array on behalf of the parent.
    $this->fields = isset($this->fields) ? $this->fields : array();

    $this->datasource = searchlight_get_datasource($base_table);
    $this->datasource->init();
    $this->datasource_id = $this->datasource->id;

    $this->backend = $backend;
    $this->client = $backend->initClient($this->datasource);

    $this->offset = NULL;
    $this->limit = NULL;
    $this->search_buildmode = NULL;
    $this->search_facets = array();
    $this->search_filter = array();
    $this->search_options = array();
    $this->search_result = array();
    $this->search_sort = array();
    $this->search_query = '';

    // This flag lets others know we are a searchlight-based view.
    $this->searchlight = TRUE;
  }
  /**
   * Override of add_where().
   */
  function add_where($group, $field, $value = NULL, $operator = NULL) {
    // Call the parent method to add the filter SQL side.
    parent::add_where($group, $field, $value, $operator);

    foreach ($this->datasource->fields as $name => $info) {
      $match = str_replace(".", "_", $field);
      if ($name === $match || "{$info['table']}_{$info['field']}" === $match) {
        $this->search_filter[] = array(
          'field' => $name,
          'operator' => !empty($operator) ? strtoupper($operator) : 'IN',
          'args' => $value
        );
        return;
      }
    }
  }

  function add_orderby($table, $field, $order, $alias = '', $params = array()) {
    // Call the parent method to add the orderby SQL side.
    parent::add_orderby($table, $field, $order, $alias, $params);

    if ($field === 'searchlight_weight') {
      $this->search_sort[] = array(
        'field' => 'searchlight_weight',
        'direction' => $order,
      );
    }
    else {
      // Field is aliased. Use query fields to retrieve actual table, field.
      if (!empty($alias) && isset($this->fields[$alias])) {
        $table = $this->fields[$alias]['table'];
        $field = $this->fields[$alias]['field'];
      }
      // Use table field info to retrieve datasource field name and add sort.
      if (isset($table, $field)) {
        foreach ($this->datasource->fields as $name => $info) {
          if ($name === "{$table}_{$field}" && $info['usage'] !== 'content') {
            $this->search_sort[] = array(
              'field' => $name,
              'direction' => $order,
            );;
            break;
          }
        }
      }
      // If no table was specified or found, this is probably a function.
      // Use the alias directly if it is available.
      else if (empty($table) && isset($this->datasource->fields[$alias]) && $this->datasource->fields[$alias]['usage'] !== 'content') {
        $this->search_sort[] = array(
          'field' => $alias,
          'direction' => $order,
        );
      }
    }
  }

  function add_search_facet($name, $limit = 5, $options = array()) {
    $this->search_facets[] = array(
      'field' => $name,
      'limit' => $limit,
    ) + $options;
  }

  /**
   * Set the Search build mode behavior.
   *
   * @param string $mode
   *   One of the following modes:
   *   'default': Build the Search for facets, etc. but use the View's default
   *              result set.
   *   'search':  Build the Search for facets, etc. and use the search backend
   *              to generate the result set.
   *   'empty':   Build the Search for facets, etc. but blank this View's
   *              result set.
   * @param boolean $force
   *   Force the mode specified. Otherwise, will only set the build mode if no
   *   other mode has been set yet.
   */
  function set_search_buildmode($mode = 'default', $force = FALSE) {
    if (!isset($this->search_buildmode) || $force) {
      $this->search_buildmode = $mode;
    }
  }

  function set_search_options($options) {
    $this->search_options = $options;
  }

  function set_search_query($query) {
    $this->search_query = $query;
  }

  function get_search_facet($facet) {
    return isset($this->search_result_facets[$facet]) ? $this->search_result_facets[$facet] : array();
  }

  protected function get_args($args, $offset = 2) {
    $args = array_slice($args, $offset);
    if (count($args) == 1 && is_array(reset($args))) {
      return current($args);
    }
    return $args;
  }

  function build(&$view) {
    $this->set_search_buildmode();

    // Fail the rest of the build entirely if the hideEmpty option is true and
    // there is no search query available. We don't exit right here in order
    // to allow facet queries to run.
    if (empty($this->fields) || $this->search_buildmode === 'empty') {
      parent::add_where(0, "FALSE");
    }

    // Views pager initialization.
    if ($this->search_buildmode === 'search') {
      // Views 3.x-alpha3 and prior.
      if (method_exists($this, 'init_pager')) {
        $this->init_pager($view);
      }
      // Views 3.x dev
      else if (method_exists($view, 'init_pager')) {
        $view->init_pager();
      }
      if (!empty($this->pager)) {
        if ($this->pager->use_pager()) {
          $this->pager->set_current_page($view->current_page);
        }
        // Let the pager modify the query to add limits.
        $this->pager->query();
      }
    }

    // Views query token replacements.
    $replacements = module_invoke_all('views_query_substitutions', $view);

    // Do token replacement against filter fields & args.
    if (!empty($replacements)) {
      foreach ($this->search_filter as $j => $filter) {
        $this->search_filter[$j]['field'] = str_replace(array_keys($replacements), $replacements, $this->search_filter[$j]['field']);
        if (is_array($this->search_filter[$j]['args'])) {
          foreach ($this->search_filter[$j]['args'] as $k => $arg) {
            $this->search_filter[$j]['args'][$k] = str_replace(array_keys($replacements), $replacements, $arg);
          }
        }
        else {
          $this->search_filter[$j]['args'] = str_replace(array_keys($replacements), $replacements, $this->search_filter[$j]['args']);
        }
      }
    }

    // Set backend client options.
    $this->backend->setOptions($this->client, $this->datasource, $this->search_options);
    $this->backend->setSort($this->client, $this->datasource, $this->search_sort);
    $this->backend->setFilter($this->client, $this->datasource, $this->search_filter);
    $this->backend->setPager($this->client, $this->offset, $this->limit);
    if ($this->base_table === 'node' && !user_access('administer nodes') && !empty($this->datasource->options['node_access'])) {
      $this->backend->setNodeAccess($this->client, node_access_grants('view'));
    }

    // Main query execution.
    $this->search_result = $this->backend->executeQuery($this->client, $this->datasource, $this->search_query);

    // Build facets.
    $this->search_result_facets = $this->backend->facetBuild($this->client, $this->datasource, $this->search_query, $this->search_facets);

    if ($this->search_buildmode === 'search') {
      if ($this->search_result) {
        // Make the query distinct if the option was set.
        if (!empty($this->options['distinct'])) {
          $this->set_distinct();
        }

        parent::add_where(0, "{$this->base_table}.{$this->base_field}", $this->search_result['result'], 'IN');

        $view->build_info['query'] = $this->query();
        $view->build_info['count_query'] = $this->query();
      }
      else {
        // we did not return any search results. Return no result.
        $this->search_buildmode = 'empty';
      }
    }
    elseif ($this->search_buildmode === 'default') {
      parent::build($view);
    }
  }

  function execute(&$view) {
    if ($this->search_buildmode === 'empty') {
      //
    }
    elseif ($this->search_buildmode === 'default') {
      parent::execute($view);
    }
    elseif ($this->search_buildmode === 'search') {
      // Store values prior to execute().
      $offset = $this->offset;
      $limit = $this->limit;
      $current_page = $this->pager->current_page;
      $this->offset = $this->limit = 0;

      parent::execute($view);
      if (!empty($this->search_result)) {
        $view->total_rows = $this->pager->total_items = $this->search_result['total'];
        $this->pager->set_current_page($current_page);
        $this->pager->update_page_info();
      }

      // Restore original values.
      $this->offset = $offset;
      $this->limit = $limit;

      if ($view->built && !empty($this->search_result)) {
        // Ensure the order of the result set from Views matches that of the
        // search backend. Don't attempt to do this for aggregate queries.
        if (empty($this->has_aggregate)) {
          $sortmap = array();
          $positions = array_flip(array_values($this->search_result['result']));
          $i = count($positions);
          foreach ($view->result as $num => $row) {
            $key = $row->{$view->base_field};
            // If in search results use its position in the resultset.
            if (isset($positions[$key])) {
              $sortmap[$num] = $positions[$key];
            }
            // If not, move to the end of the stack.
            else {
              $sortmap[$num] = $i;
              $i++;
            }
          }
          array_multisort($sortmap, $view->result);
        }
      }
    }
  }
}

