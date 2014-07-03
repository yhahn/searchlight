// $Id$
(function ($) {
  Drupal.behaviors.searchlight = {
    attach: function(context, settiongs) {
      $('.searchlight-backend-select:not(.searchlight-processed)', context).each(function() {
        $(this).change(function() {
          var value = $(this).val();
          $('.searchlight-backend-settings').hide();
          $('.searchlight-backend-' + value).show();
        });
        $(this).change();
      }).addClass('searchlight-processed');
      $('.searchlight-admin-environment .environment-settings:not(.searchlight-processed)', context).each(function() {
        $('a.environment-settings-link', this).click(function() {
          if ($(this).is('.settings-active')) {
            $('.searchlight-admin-environment .environment-settings-form').hide();
            $('a.environment-settings-link').removeClass('settings-active');
          }
          else {
            // Hide & show per-facet settings forms.
            $('.searchlight-admin-environment .environment-settings-form').hide();
            var target = $(this).attr('href').split('#')[1];
            $('#' + target).show();

            // Set link classes.
            $('a.environment-settings-link').removeClass('settings-active');
            $(this).addClass('settings-active');
          }
        });
        $(this).change();
      }).addClass('searchlight-processed');
    }
  }
})(jQuery);
