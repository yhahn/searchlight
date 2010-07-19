#############################################################################
# datasource: <?php print $datasource['conf']['id'] ?>
#############################################################################

source <?php print $datasource['conf']['id'] ?>
{
type = <?php print $datasource['conf']['type'] ?>
sql_host = <?php print $datasource['conf']['sql_host'] ?>
sql_user = <?php print $datasource['conf']['sql_user'] ?>
sql_pass = <?php print $datasource['conf']['sql_pass'] ?>
sql_db   = <?php print $datasource['conf']['sql_db'] ?>
sql_port = <?php print $datasource['conf']['sql_port'] ?>
<?php if (!empty($datasource['conf']['sql_sock'] )): ?>
sql_sock = <?php print $datasource['conf']['sql_sock'] ?>
<?php endif; ?>

sql_query_pre = \
  <?php print $datasource['conf']['sql_query_pre'] ?>
sql_query = \
  <?php print $datasource['conf']['sql_query'] ?>
sql_query_info = \
  <?php print $datasource['conf']['sql_query_info'] ?>
sql_query_range = <?php print $datasource['conf']['sql_query_range'] ?>
sql_range_step = <?php print $datasource['conf']['sql_range_step'] ?>
sql_ranged_throttle = <?php print $datasource['conf']['sql_ranged_throttle'] ?>

<?php print $datasource['conf']['sql_attr'] ?>
}

#############################################################################
# index: <?php print $datasource['conf']['id'] ?>
#############################################################################

index <?php print $datasource['conf']['id'] ?>
{
# Index configuration
source = <?php print $datasource['conf']['id'] ?>
<?php foreach ($datasource['index'] as $key => $value) :  ?>
  <?php print $key ?> = <?php print $datasource['index'][$key] ?>
<?php endforeach; ?>
} 


