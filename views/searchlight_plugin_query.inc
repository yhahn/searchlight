<?php

class searchlight_plugin_query extends views_plugin_query_default {
  var $backend;
  var $client;
  var $datasource;
  var $datasource_id;
  var $search_filter;
  var $search_options;
  var $search_result;
  var $search_sort;
  var $search_query;
  var $searchlight;

  /**
   * Override of init().
   */
  function init($base_table = 'node', $base_field = 'nid', $backend) {
    parent::init($base_table, $base_field);

    $this->backend = $backend;
    $this->client = $backend->initClient();

    $this->datasource = searchlight_get_datasource($base_table);
    $this->datasource->init();
    $this->datasource_id = $this->datasource->id;

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
  function add_where($group, $clause) {
    $where_args = func_get_args();
    $where_args = $this->get_args($where_args);

    $split = preg_split('/[ ]([<>=!]*)|(IN)|(NOT IN)/i', trim($clause), NULL, PREG_SPLIT_NO_EMPTY | PREG_SPLIT_DELIM_CAPTURE);
    if (count($split) >= 2) {
      $field = explode('.', trim(array_shift($split)));
      $operator = strtoupper(trim(array_shift($split)));
      if (count($field) === 2) {
        $table = $field[0];
        $field = $field[1];
        foreach ($this->datasource->fields as $name => $info) {
          if ($info['table'] === $table && ($info['name'] === $field || $info['field'] === $field)) {
            // Get the arguments for this where clause.
            $args = array();
            $argnum = substr_count($clause, '%');
            if (count($where_args) >= $argnum) {
              while ($argnum > 0) {
                $args[] = array_shift($where_args);
                $argnum--;
              }
            }

            // Done.
            $this->search_filter[] = array('field' => $name, 'operator' => $operator, 'args' => $args);
            break;
          }
        }
      }
    }
  }

  function add_orderby($table, $field, $order, $alias = '', $params = array()) {
    if ($field === 'searchlight_weight') {
      $this->search_sort[] = array('field' => 'searchlight_weight', 'order' => $order); // "@weight {$order}";
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
          if ($info['table'] === $table && $info['field'] === $field) {
            $this->search_sort[] = array('field' => $name, 'order' => $order); // "{$name} {$order}";
            break;
          }
        }
      }
    }
  }

  function build(&$view) {
    // Fail the build entirely if the hideEmpty option is true and there is no
    // search query available.
    if (!empty($this->search_options['hideEmpty']) && empty($this->search_query)) {
      return;
    }
    $this->init_pager($view);
    if ($this->pager->use_pager()) {
      $this->pager->set_current_page($view->current_page);
    }
    // Let the pager modify the query to add limits.
    $this->pager->query();

    // Views query token replacements.
    $replacements = module_invoke_all('views_query_substitutions', $view);

    // Do token replacement against filter fields & args.
    if (!empty($replacements)) {
      foreach ($this->search_filter as $j => $filter) {
        $this->search_filter[$j]['field'] = str_replace(array_keys($replacements), $replacements, $this->search_filter[$j]['field']);
        foreach ($this->search_filter[$j]['args'] as $k => $arg) {
          $this->search_filter[$j]['args'][$k] = str_replace(array_keys($replacements), $replacements, $arg);
        }
      }
    }

    // Execute the search.
    $this->backend->setOptions($this->client, $this->datasource, $this->search_options);
    $this->backend->setSort($this->client, $this->datasource, $this->search_sort);
    $this->backend->setFilter($this->client, $this->datasource, $this->search_filter);
    $this->backend->setPager($this->client, $this->offset, $this->limit);

    if ($this->base_table === 'node' && !user_access('administer nodes')) {
      $this->backend->setNodeAccess($this->client, node_access_grants('view'));
    }

    if ($this->search_result = $this->backend->executeQuery($this->client, $this->datasource, $this->search_query)) {
      // Clear out normal field, sort, where portions of the query that
      // have been taken care of by the backend.
      $this->orderby = array();
      $this->where = array();

      // @TODO: do this with placeholders, args.
      $ids = implode(',', $this->search_result['result']);
      parent::add_where(0, "{$this->base_table}.{$this->base_field} IN ({$ids})");

      $view->build_info['query'] = $this->query();
      $view->build_info['count_query'] = "SELECT FALSE";
      $view->build_info['query_args'] = $this->get_where_args();
    }
  }

  function set_search_options($options) {
    $this->search_options = $options;
  }

  function set_search_query($query) {
    $this->search_query = $query;
  }

  protected function get_args($args, $offset = 2) {
    $args = array_slice($args, $offset);
    if (count($args) == 1 && is_array(reset($args))) {
      return current($args);
    }
    return $args;
  }

  function execute(&$view) {
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
  }
}
