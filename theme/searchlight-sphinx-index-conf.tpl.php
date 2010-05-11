#############################################################################
# datasource: <?php print $id ?>
#############################################################################

source <?php print $id ?>
{
type = <?php print $type ?>
sql_host = <?php print $sql_host ?>
sql_user = <?php print $sql_user ?>
sql_pass = <?php print $sql_pass ?>
sql_db   = <?php print $sql_db ?>
sql_port = <?php print $sql_port ?>
<?php if (!empty($sql_sock )): ?>
sql_sock = <?php print $sql_sock ?>
<?php endif; ?>

sql_query = \
  <?php print $sql_query ?>
sql_query_info = \
  <?php print $sql_query_info ?>
sql_query_range = <?php print $sql_query_range ?>
sql_range_step = <?php print $sql_range_step ?>
sql_ranged_throttle = <?php print $sql_ranged_throttle ?>

<?php print $sql_attr ?>
}

#############################################################################
# index: <?php print $id ?>
#############################################################################

index <?php print $id ?>
{
# Index configuration
source = <?php print $id ?>
<?php foreach ($index as $key => $value) :  ?>
  <?php print $key ?> = <?php print $index[$key] ?>
<?php endforeach; ?>
} 


