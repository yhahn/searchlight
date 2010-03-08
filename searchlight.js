// $Id$
Drupal.behaviors.searchlight = function(context) {
  $('.searchlight-backend-select:not(.searchlight-processed)', context).each(function() {
    $(this).change(function() {
      var value = $(this).val();
      $('.searchlight-backend-settings').hide();
      $('.searchlight-backend-' + value).show();
    });
    $(this).change();
  }).addClass('searchlight-processed');
};
