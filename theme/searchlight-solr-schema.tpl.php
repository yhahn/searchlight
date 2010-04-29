<?php print '<?xml version="1.0" encoding="UTF-8" ?>'; ?>
<schema name="drupal-0.9.5" version="1.2">
  <types>
    <fieldType name="string" class="solr.StrField" sortMissingLast="true" omitNorms="true"/>
    <fieldType name="boolean" class="solr.BoolField" sortMissingLast="true" omitNorms="true"/>
    <fieldType name="integer" class="solr.IntField" omitNorms="true"/>
    <fieldType name="long" class="solr.LongField" omitNorms="true"/>
    <fieldType name="float" class="solr.FloatField" omitNorms="true"/>
    <fieldType name="double" class="solr.DoubleField" omitNorms="true"/>
    <fieldType name="sint" class="solr.SortableIntField" sortMissingLast="true" omitNorms="true"/>
    <fieldType name="slong" class="solr.SortableLongField" sortMissingLast="true" omitNorms="true"/>
    <fieldType name="sfloat" class="solr.SortableFloatField" sortMissingLast="true" omitNorms="true"/>
    <fieldType name="sdouble" class="solr.SortableDoubleField" sortMissingLast="true" omitNorms="true"/>
    <fieldType name="tint" class="solr.TrieIntField" precisionStep="8" omitNorms="true" positionIncrementGap="0"/>
    <fieldType name="tfloat" class="solr.TrieFloatField" precisionStep="8" omitNorms="true" positionIncrementGap="0"/>
    <fieldType name="tlong" class="solr.TrieLongField" precisionStep="8" omitNorms="true" positionIncrementGap="0"/>
    <fieldType name="tdouble" class="solr.TrieDoubleField" precisionStep="8" omitNorms="true" positionIncrementGap="0"/>
    <fieldType name="date" class="solr.DateField" sortMissingLast="true" omitNorms="true"/>
    <fieldType name="tdate" class="solr.TrieDateField" omitNorms="true" precisionStep="6" positionIncrementGap="0"/>
    <fieldType name="text_ws" class="solr.TextField" positionIncrementGap="100">
      <analyzer>
        <tokenizer class="solr.WhitespaceTokenizerFactory"/>
      </analyzer>
    </fieldType>
    <fieldType name="text" class="solr.TextField" positionIncrementGap="100">
      <analyzer type="index">
        <charFilter class="solr.MappingCharFilterFactory" mapping="mapping-ISOLatin1Accent.txt"/>
        <tokenizer class="solr.WhitespaceTokenizerFactory"/>
        <filter class="solr.StopFilterFactory"
                ignoreCase="true"
                words="stopwords.txt"
                enablePositionIncrements="true"
                />
        <filter class="solr.WordDelimiterFilterFactory" generateWordParts="1" generateNumberParts="1" catenateWords="1" catenateNumbers="1" catenateAll="0" splitOnCaseChange="1"/>
        <filter class="solr.LowerCaseFilterFactory"/>
        <filter class="solr.SnowballPorterFilterFactory" language="English" protected="protwords.txt"/>
        <filter class="solr.RemoveDuplicatesTokenFilterFactory"/>
      </analyzer>
      <analyzer type="query">
        <charFilter class="solr.MappingCharFilterFactory" mapping="mapping-ISOLatin1Accent.txt"/>
        <tokenizer class="solr.WhitespaceTokenizerFactory"/>
        <filter class="solr.SynonymFilterFactory" synonyms="synonyms.txt" ignoreCase="true" expand="true"/>
        <filter class="solr.StopFilterFactory"
                ignoreCase="true"
                words="stopwords.txt"
                enablePositionIncrements="true"
                />
        <filter class="solr.WordDelimiterFilterFactory" generateWordParts="1" generateNumberParts="1" catenateWords="0" catenateNumbers="0" catenateAll="0" splitOnCaseChange="1"/>
        <filter class="solr.LowerCaseFilterFactory"/>
        <filter class="solr.SnowballPorterFilterFactory" language="English" protected="protwords.txt"/>
        <filter class="solr.RemoveDuplicatesTokenFilterFactory"/>
      </analyzer>
    </fieldType>
    <fieldType name="edge_n2_kw_text" class="solr.TextField" positionIncrementGap="100">
     <analyzer type="index">
       <tokenizer class="solr.KeywordTokenizerFactory"/>
       <filter class="solr.LowerCaseFilterFactory"/>
       <filter class="solr.EdgeNGramFilterFactory" minGramSize="2" maxGramSize="25" />
     </analyzer>
     <analyzer type="query">
       <tokenizer class="solr.KeywordTokenizerFactory"/>
       <filter class="solr.LowerCaseFilterFactory"/>
     </analyzer>
    </fieldType>
   <fieldType name="textSpell" class="solr.TextField" positionIncrementGap="100">
     <analyzer>
       <tokenizer class="solr.StandardTokenizerFactory" />
       <filter class="solr.StopFilterFactory" ignoreCase="true" words="stopwords.txt"/>
       <filter class="solr.LengthFilterFactory" min="4" max="20" />
       <filter class="solr.LowerCaseFilterFactory" /> 
       <filter class="solr.RemoveDuplicatesTokenFilterFactory" /> 
     </analyzer>
   </fieldType>
    <fieldType name="sortString" class="solr.TextField" sortMissingLast="true" omitNorms="true">
      <analyzer>
        <tokenizer class="solr.KeywordTokenizerFactory"/>
        <filter class="solr.LowerCaseFilterFactory" />
        <filter class="solr.TrimFilterFactory" />
      </analyzer>
    </fieldType>
    <fieldType name="rand" class="solr.RandomSortField" indexed="true" />
    <fieldtype name="ignored" stored="false" indexed="false" class="solr.StrField" />
 </types>

 <fields>
<?php /*
   <field name="id" type="string" indexed="true" stored="true" required="true" />
   <field name="site" type="string" indexed="true" stored="true"/>
   <field name="hash" type="string" indexed="true" stored="true"/>
   <field name="url" type="string" indexed="true" stored="true"/>
   <field name="title" type="text" indexed="true" stored="true" termVectors="true" omitNorms="true"/>
   <field name="sort_title" type="sortString" indexed="true" stored="false"/>
   <field name="body" type="text" indexed="true" stored="true" termVectors="true"/>
   <field name="teaser" type="text" indexed="false" stored="true"/>
   <!-- entity is 'node', 'file', 'user', or some other Drupal object type -->
   <field name="entity" type="string" indexed="true" stored="true"/>
   <!-- type is a node type, or can be used flexibly for other entity types -->
   <field name="type" type="string" indexed="true" stored="true"/>
   <field name="type_name" type="string" indexed="true" stored="true"/>
   <field name="path" type="string" indexed="true" stored="true"/>
   <field name="path_alias" type="text" indexed="true" stored="true" termVectors="true"/>
   <field name="uid"  type="integer" indexed="true" stored="true"/>
   <field name="name" type="text" indexed="true" stored="true" termVectors="true"/>
   <field name="sname" type="string" indexed="true" stored="false"/>
   <field name="sort_name" type="sortString" indexed="true" stored="false"/>
   <field name="created" type="date" indexed="true" stored="true"/>
   <field name="changed" type="date" indexed="true" stored="true"/>
   <field name="last_comment_or_change" type="date" indexed="true" stored="true"/>
   <field name="nid"  type="integer" indexed="true" stored="true"/>
   <field name="status" type="boolean" indexed="true" stored="true"/>
   <field name="promote" type="boolean" indexed="true" stored="true"/>
   <field name="moderate" type="boolean" indexed="true" stored="true"/>
   <field name="sticky" type="boolean" indexed="true" stored="true"/>
   <field name="tnid"  type="integer" indexed="true" stored="true"/>
   <field name="translate" type="boolean" indexed="true" stored="true"/>
   <field name="language" type="string" indexed="true" stored="true"/>
   <field name="comment_count" type="integer" indexed="true" stored="true"/>
   <field name="tid"  type="integer" indexed="true" stored="true" multiValued="true"/>
   <field name="vid"  type="integer" indexed="true" stored="true" multiValued="true"/>
   <field name="taxonomy_names" type="text" indexed="true" stored="false" termVectors="true" multiValued="true" omitNorms="true"/>
   <!-- The string version of the title is used for sorting -->
   <copyField source="title" dest="sort_title"/>
   <!-- The string versions of the name used for sorting/multi-site facets -->
   <copyField source="name" dest="sname"/>
   <copyField source="name" dest="sort_name"/>
   <!-- Copy terms to a single field that contains all taxonomy term names -->
   <copyField source="ts_vid_*" dest="taxonomy_names"/>
  
   <!-- A set of fields to contain text extracted from tag contents which we
        can boost at query time. -->
   <field name="tags_h1" type="text" indexed="true" stored="false" omitNorms="true"/>
   <field name="tags_h2_h3" type="text" indexed="true" stored="false" omitNorms="true"/>
   <field name="tags_h4_h5_h6" type="text" indexed="true" stored="false" omitNorms="true"/>
   <field name="tags_a" type="text" indexed="true" stored="false" omitNorms="true"/>
   <!-- Inline tags are typically u, b, i, em, strong -->
   <field name="tags_inline" type="text" indexed="true" stored="false" omitNorms="true"/>

   <!-- Here, default is used to create a "timestamp" field indicating
        when each document was indexed.-->
   <field name="timestamp" type="date" indexed="true" stored="true" default="NOW" multiValued="false"/>

	<!-- This field is used to build the spellchecker index -->
   <field name="spell" type="textSpell" indexed="true" stored="true" multiValued="true"/>
   <copyField source="title" dest="spell"/>
   <copyField source="body" dest="spell"/>

   <dynamicField name="is_*"  type="integer" indexed="true"  stored="true" multiValued="false"/>
   <dynamicField name="im_*"  type="integer" indexed="true"  stored="true" multiValued="true"/>
   <dynamicField name="sis_*" type="sint"    indexed="true"  stored="true" multiValued="false"/>
   <dynamicField name="sim_*" type="sint"    indexed="true"  stored="true" multiValued="true"/>
   <dynamicField name="sm_*"  type="string"    indexed="true"  stored="true" multiValued="true"/>
   <dynamicField name="tm_*"  type="text"    indexed="true"  stored="true" multiValued="true" termVectors="true"/>
   <dynamicField name="ss_*"  type="string"    indexed="true"  stored="true" multiValued="false"/>
   <dynamicField name="ts_*"  type="text"    indexed="true"  stored="true" multiValued="false" termVectors="true"/>
   <dynamicField name="tsen2k_*" type="edge_n2_kw_text" indexed="true" stored="true" multiValued="false" omitNorms="true" omitTermFreqAndPositions="true" />
   <dynamicField name="ds_*" type="date"    indexed="true"  stored="true" multiValued="false"/>
   <dynamicField name="dm_*" type="date"    indexed="true"  stored="true" multiValued="true"/>
   <dynamicField name="tds_*" type="tdate"    indexed="true"  stored="true" multiValued="false"/>
   <dynamicField name="tdm_*" type="tdate"    indexed="true"  stored="true" multiValued="true"/>
   <dynamicField name="bm_*"  type="boolean" indexed="true"  stored="true" multiValued="true"/>
   <dynamicField name="bs_*"  type="boolean" indexed="true"  stored="true" multiValued="false"/>
   <dynamicField name="fs_*"  type="sfloat"  indexed="true"  stored="true" multiValued="false"/>
   <dynamicField name="fm_*"  type="sfloat"  indexed="true"  stored="true" multiValued="true"/>
   <dynamicField name="ps_*"  type="sdouble" indexed="true"  stored="true" multiValued="false"/>
   <dynamicField name="pm_*"  type="sdouble" indexed="true"  stored="true" multiValued="true"/>

   <dynamicField name="tis_*"  type="tint"  indexed="true"  stored="true" multiValued="false"/>
   <dynamicField name="tim_*"  type="tint"  indexed="true"  stored="true" multiValued="true"/>
   <dynamicField name="tls_*"  type="tlong" indexed="true"  stored="true" multiValued="false"/>
   <dynamicField name="tlm_*"  type="tlong" indexed="true"  stored="true" multiValued="true"/>
   <dynamicField name="tfs_*"  type="tfloat"  indexed="true"  stored="true" multiValued="false"/>
   <dynamicField name="tfm_*"  type="tfloat"  indexed="true"  stored="true" multiValued="true"/>
   <dynamicField name="tps_*"  type="tdouble" indexed="true"  stored="true" multiValued="false"/>
   <dynamicField name="tpm_*"  type="tdouble" indexed="true"  stored="true" multiValued="true"/>
   <!-- Sortable version of the dynamic string field -->
   <dynamicField name="sort_ss_*" type="sortString" indexed="true" stored="false"/>
   <copyField source="ss_*" dest="sort_ss_*"/>
  <!-- A random sort field -->
   <dynamicField name="random_*" type="rand" indexed="true" stored="true"/>
   <!-- This field is used to store node access records, as opposed to CCK field data -->
   <dynamicField name="nodeaccess*" type="integer" indexed="true" stored="false" multiValued="true"/>

   <!-- The following causes solr to ignore any fields that don't already match an existing
        field name or dynamic field, rather than reporting them as an error.
        Alternately, change the type="ignored" to some other type e.g. "text" if you want
        unknown fields indexed and/or stored by default -->
   <dynamicField name="*" type="ignored" multiValued="true" />
 */
?>
  <?php foreach($datasource['schema'] as $f): ?>
    <field name="<?php print $f['name'];?>"  type="<?php print $f['type'];?>" indexed="<?php print $f['indexed'];?>" stored="<?php print $f['stored'];?>" multiValued="<?php print $f['multiValued'];?>"/>
  <?php endforeach; ?>
 </fields>
 <uniqueKey><?php print $datasource['conf']['unique_key']; ?></uniqueKey>

<?php /*
 <!-- field for the QueryParser to use when an explicit fieldname is absent -->
 <defaultSearchField>body</defaultSearchField>
*/ ?>

 <!-- SolrQueryParser configuration: defaultOperator="AND|OR" -->
 <solrQueryParser defaultOperator="AND"/>

</schema>
