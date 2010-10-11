<?php

/**
 * Helper to implementation of hook_views_default_views().
 */
function _searchlight_basic_views_default_views() {
  $views = array();

  // Exported view: search_node
  $view = new view;
  $view->name = 'search_node';
  $view->description = 'Basic search.';
  $view->tag = 'search';
  $view->view_php = '';
  $view->base_table = 'node';
  $view->is_cacheable = FALSE;
  $view->api_version = '3.0-alpha1';
  $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */

  /* Display: Defaults */
  $handler = $view->new_display('default', 'Defaults', 'default');
  $handler->display->display_options['access']['type'] = 'none';
  $handler->display->display_options['cache']['type'] = 'none';
  $handler->display->display_options['exposed_form']['type'] = 'basic';
  $handler->display->display_options['pager']['type'] = 'full';
  $handler->display->display_options['style_plugin'] = 'list';
  $handler->display->display_options['row_plugin'] = 'fields';
  /* Field: User: Picture */
  $handler->display->display_options['fields']['picture']['id'] = 'picture';
  $handler->display->display_options['fields']['picture']['table'] = 'users';
  $handler->display->display_options['fields']['picture']['field'] = 'picture';
  $handler->display->display_options['fields']['picture']['label'] = '';
  $handler->display->display_options['fields']['picture']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['picture']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['picture']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['picture']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['picture']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['picture']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['picture']['alter']['html'] = 0;
  /* Field: User: Name */
  $handler->display->display_options['fields']['name']['id'] = 'name';
  $handler->display->display_options['fields']['name']['table'] = 'users';
  $handler->display->display_options['fields']['name']['field'] = 'name';
  $handler->display->display_options['fields']['name']['label'] = '';
  $handler->display->display_options['fields']['name']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['name']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['name']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['name']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['name']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['name']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['name']['alter']['html'] = 0;
  $handler->display->display_options['fields']['name']['link_to_user'] = 1;
  $handler->display->display_options['fields']['name']['overwrite_anonymous'] = 0;
  /* Field: Node: Post date */
  $handler->display->display_options['fields']['created']['id'] = 'created';
  $handler->display->display_options['fields']['created']['table'] = 'node';
  $handler->display->display_options['fields']['created']['field'] = 'created';
  $handler->display->display_options['fields']['created']['label'] = '';
  $handler->display->display_options['fields']['created']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['created']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['created']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['created']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['created']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['created']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['created']['alter']['html'] = 0;
  $handler->display->display_options['fields']['created']['date_format'] = 'reldate';
  /* Field: Node: Title */
  $handler->display->display_options['fields']['title']['id'] = 'title';
  $handler->display->display_options['fields']['title']['table'] = 'node';
  $handler->display->display_options['fields']['title']['field'] = 'title';
  $handler->display->display_options['fields']['title']['label'] = '';
  $handler->display->display_options['fields']['title']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['title']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['title']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['title']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['title']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['title']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['title']['alter']['html'] = 0;
  $handler->display->display_options['fields']['title']['link_to_node'] = 1;
  /* Field: Node: Teaser */
  /*
  $handler->display->display_options['fields']['teaser']['id'] = 'teaser';
  $handler->display->display_options['fields']['teaser']['table'] = 'node_revisions';
  $handler->display->display_options['fields']['teaser']['field'] = 'teaser';
  $handler->display->display_options['fields']['teaser']['label'] = '';
  $handler->display->display_options['fields']['teaser']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['teaser']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['teaser']['alter']['trim'] = 1;
  $handler->display->display_options['fields']['teaser']['alter']['max_length'] = '150';
  $handler->display->display_options['fields']['teaser']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['teaser']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['teaser']['alter']['strip_tags'] = 1;
  $handler->display->display_options['fields']['teaser']['alter']['html'] = 0;
  */
  /* Sort criterion: Search: Searchlight */
  $handler->display->display_options['sorts']['search']['id'] = 'search';
  $handler->display->display_options['sorts']['search']['table'] = 'searchlight';
  $handler->display->display_options['sorts']['search']['field'] = 'search';
  $handler->display->display_options['sorts']['search']['order'] = 'DESC';
  /* Filter: Search: Searchlight */
  $handler->display->display_options['filters']['search']['id'] = 'search';
  $handler->display->display_options['filters']['search']['table'] = 'searchlight';
  $handler->display->display_options['filters']['search']['field'] = 'search';
  $handler->display->display_options['filters']['search']['exposed'] = TRUE;
  $handler->display->display_options['filters']['search']['expose']['label'] = 'Search';
  $handler->display->display_options['filters']['search']['expose']['identifier'] = 'search';
  $handler->display->display_options['filters']['search']['hideEmpty'] = 1;
  $handler->display->display_options['filters']['search']['matchMode'] = '0';
  /* Filter: Search: Active facets */
  $handler->display->display_options['filters']['facets']['id'] = 'facets';
  $handler->display->display_options['filters']['facets']['table'] = 'searchlight';
  $handler->display->display_options['filters']['facets']['field'] = 'facets';

  /* Display: Page: search */
  $handler = $view->new_display('page', 'Page: search', 'page_1');
  $handler->display->display_options['path'] = 'search';

  $views[$view->name] = $view;

  return $views;
}
