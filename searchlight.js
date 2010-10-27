(function($) {

Drupal.behaviors.searchlight = {};
Drupal.behaviors.searchlight.attach = function(context, settings) {
  if (Drupal.settings && Drupal.settings.views && Drupal.settings.views.ajaxViews) {
    $('.searchlight-environment:not(.searchlightProcessed)').each(function() {
      // Retrieve current page view's settings and DOM element.
      var settings = {};
      var view = '';
      var identifier = $(this).attr('class').split('searchlight-view-')[1].split('-');
      for (var i in Drupal.settings.views.ajaxViews) {
        if (
          Drupal.settings.views.ajaxViews[i].view_name == identifier[0] &&
          Drupal.settings.views.ajaxViews[i].view_display_id == identifier[1]
        ) {
          settings = Drupal.settings.views.ajaxViews[i];
          view = $('.view-dom-id-' + settings.view_dom_id);
          $('a', this).drupalSearchlight('ajaxViewLink', {view: view, settings: settings});
          break;
        }
      }
    }).addClass('searchlightProcessed');
  }
};

$.fn.drupalSearchlight = function(method, params) {
  switch (method) {
    case 'ajaxViewLink':
      var settings = params.settings;
      var view = params.view;

      // If there are multiple views this might've ended up showing up multiple times.
      var ajax_path = Drupal.settings.views.ajax_path;
      if (ajax_path.constructor.toString().indexOf("Array") != -1) {
        ajax_path = ajax_path[0];
      }

      $(this).each(function() {
        var viewData = {};
        var link = $(this);

        // Construct an object using the settings defaults and then overriding
        // with data specific to the link.
        $.extend(
          viewData,
          Drupal.Views.parseQueryString(link.attr('href')),
          // Extract argument data from the URL.
          Drupal.Views.parseViewArgs(link.attr('href'), settings.view_base_path),
          // Settings must be used last to avoid sending url aliases to the server.
          settings
        );

        var ajax = new Drupal.ajax(false, link, {
          url: ajax_path,
          submit: viewData,
          event: 'click',
          selector: view
        });

        // Override HTTP method type and beforeSerialize method.
        // See note above.
        ajax.options.type = 'GET';
        ajax.beforeSerialize = function(element_settings, options) { return; };
      });
      break;
  }
  return this;
};

$.fn.drupalSearchlight.replace = function(selector, data) {
  if (data.searchlightData) {
    for (var target in data.searchlightData) {
      if ($(target).size()) {
        $(target).replaceWith(data.searchlightData[target]);
        Drupal.attachBehaviors($(target));
      }
    }
  }
};

})(jQuery);

