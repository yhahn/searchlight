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
        <?php /* <charFilter class="solr.MappingCharFilterFactory" mapping="mapping-ISOLatin1Accent.txt"/> */ ?>
        <tokenizer class="solr.WhitespaceTokenizerFactory"/>
        <?php /* <filter class="solr.StopFilterFactory"
                ignoreCase="true"
                words="stopwords.txt"
                enablePositionIncrements="true"
                /> */ ?>
        <filter class="solr.WordDelimiterFilterFactory" generateWordParts="1" generateNumberParts="1" catenateWords="1" catenateNumbers="1" catenateAll="0" splitOnCaseChange="1"/>
        <filter class="solr.LowerCaseFilterFactory"/>
        <?php /* <filter class="solr.SnowballPorterFilterFactory" language="English" protected="protwords.txt"/> */ ?>
        <filter class="solr.RemoveDuplicatesTokenFilterFactory"/>
      </analyzer>
      <analyzer type="query">
        <?php /* <charFilter class="solr.MappingCharFilterFactory" mapping="mapping-ISOLatin1Accent.txt"/> */ ?>
        <tokenizer class="solr.WhitespaceTokenizerFactory"/>
        <?php /* <filter class="solr.SynonymFilterFactory" synonyms="synonyms.txt" ignoreCase="true" expand="true"/> */ ?>
        <?php /* <filter class="solr.StopFilterFactory"
                ignoreCase="true"
                words="stopwords.txt"
                enablePositionIncrements="true"
                /> */ ?>
        <filter class="solr.WordDelimiterFilterFactory" generateWordParts="1" generateNumberParts="1" catenateWords="0" catenateNumbers="0" catenateAll="0" splitOnCaseChange="1"/>
        <filter class="solr.LowerCaseFilterFactory"/>
        <?php /* <filter class="solr.SnowballPorterFilterFactory" language="English" protected="protwords.txt"/> */ ?>
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
       <?php /* <filter class="solr.StopFilterFactory" ignoreCase="true" words="stopwords.txt"/> */ ?>
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
  <?php foreach($datasource['schema'] as $f): ?>
    <field name="<?php print $f['name'];?>"  type="<?php print $f['type'];?>" indexed="<?php print $f['indexed'];?>" stored="<?php print $f['stored'];?>" multiValued="<?php print $f['multiValued'];?>"/>
  <?php endforeach; ?>
 </fields>
 <uniqueKey><?php print $datasource['conf']['unique_key']; ?></uniqueKey>

 <!-- field for the QueryParser to use when an explicit fieldname is absent -->
 <defaultSearchField><?php print $datasource['conf']['default_search_field']; ?></defaultSearchField>

 <!-- SolrQueryParser configuration: defaultOperator="AND|OR" -->
 <solrQueryParser defaultOperator="AND"/>

</schema>
