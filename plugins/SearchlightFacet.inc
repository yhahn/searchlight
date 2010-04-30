<?php

class SearchlightFacet {
  function build($query, $name, $limit) {
    $items = array();
    if ($id = searchlight_facets()->getFacet($name)) {
      $items = array($id => array('id' => $id));
    }
    else {
      $field = $query->datasource->fields[$name];
      $facet_query = drupal_clone($query);
      $items = $query->backend->facetBuild($facet_query, $field, $limit);
    }
    return $items;
  }

  function render($query, $name, $limit) {
    $items = $this->build($query, $name, $limit);
    $field = $query->datasource->fields[$name];

    if ($field['usage'] === 'multivalue') {
      $view = searchlight_build_view($field['view']);
    }
    else {
      $view = $query->datasource->view->copy();
      $view->build();
    }

    // Detect which handler is responsible for this field and build fake
    // resultset for it to process.
    if ($field['usage'] === 'multivalue') {
      foreach ($view->field as $field_handler) {
        if (in_array($field_handler->field_alias, array($field['name'], $field['label_field']['name']), TRUE)) {
          $handler = $field_handler;

          // Multivalue fields must build a new Views query in order to
          // retrieve any secondary label field values.
          $query = views_get_plugin('query', 'views_query');
          $query->init($field['table'], $field['field']);

          $ids = array_keys($items);

          // Add WHERE condition on item ids.
          $query->add_where(0, "{$field['table']}.{$field['field']} IN (". db_placeholders($ids, 'int') .")", $ids);

          // Add base field.
          $field_alias = $query->add_field($field['table'], $field['field']);

          // Add label field.
          $label_alias = $query->add_field($field['label_field']['table'], $field['label_field']['field']);
          $result = db_query($query->query(), $ids);

          $rows = array();
          while ($row = db_fetch_object($result)) {
            $row->{$field['name']} = $row->{$field['field']};
            $row->{$field['label_field']['name']} = $row->{$label_alias};
            $rows[$row->{$field['name']}] = $row;
          }
          break;
        }
      }
    }
    else {
      foreach ($view->field as $field_handler) {
        if ($field_handler->field_alias === $field['name']) {
          $handler = $field_handler;
          $rows = array();
          foreach ($items as $item) {
            $row = new stdClass();
            $row->{$field['name']} = $item['id'];
            $rows[$item['id']] = $row;
          }
          break;
        }
      }
    }
    // Fake some rows so we can use the pre_render method.
    if (!empty($handler)) {
      $handler->pre_render($rows);

      // Now render each row.
      $facets = searchlight_facets();
      $rendered = array();

      // Active facet.
      if ($facets->getFacet($field['name'])) {
        foreach ($rows as $id => $row) {
          // Generate a PURL modifier for this item.
          $modifier = $facets->getModifier('remove', $field['name'], $id);
          $item = $items[$id];
          $item['title'] = strip_tags($handler->render($row));
          if (!empty($modifier)) {
            $item['link'] = l(t('remove'), $_GET['q'], array(
              'purl' => array('provider' => 'searchlight', 'id' => $modifier))
            );
          }
          else {
            $item['link'] = l(t('remove'), $_GET['q'], array(
              'purl' => array('remove' => array('searchlight')))
            );
          }
          $rendered[$id] = theme('searchlight_facet_active', $field, $item);
        }
      }
      // Inactive facet.
      else {
        foreach ($rows as $id => $row) {
          // Generate a PURL modifier for this item.
          $modifier = $facets->getModifier('add', $field['name'], $id);
          $item = $items[$id];
          $item['title'] = strip_tags($handler->render($row));
          $item['link'] = l($item['title'], $_GET['q'], array(
            'query' => drupal_query_string_encode($_GET, array('q')),
            'purl' => array('provider' => 'searchlight', 'id' => $modifier))
          );
          $rendered[$id] = theme('searchlight_facet_link', $field, $item);
        }
      }
      return array('label' => $handler->label(), 'items' => $rendered);
    }
    return FALSE;
  }
}

