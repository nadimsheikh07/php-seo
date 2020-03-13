<?php

$crawledLinks = array();
const MAX_DEPTH = 5;

function followLink($url, $depth = 0)
{
    global $crawledLinks;
    $crawling = array();
    if ($depth > MAX_DEPTH) {
        echo "<div style='color:red;'>The Crawler is giving up!</div>";
        return;
    }
    $options = array(
        'http' => array(
            'method' => "GET",
            'user-agent' => "gfgBot/0.1\n"
        )
    );
    $context = stream_context_create($options);
    $doc = new DomDocument();
    @$doc->loadHTML(file_get_contents($url, false, $context));
    $links = $doc->getElementsByTagName('a');
    $pageTitle = getDocTitle($doc, $url);
    $metaData = getDocMetaData($doc);
    foreach ($links as $i) {
        $link = $i->getAttribute('href');
        if (ignoreLink($link)) continue;
        $link = convertLink($url, $link);
        if (!in_array($link, $crawledLinks)) {
            $crawledLinks[] = $link;
            $crawling[] = $link;
            insertIntoDatabase($link, $pageTitle, $metaData, $depth);
        }
    }
    foreach ($crawling as $crawlURL)
        followLink($crawlURL, $depth + 1);
}

function convertLink($site, $path)
{
    if (substr_compare($path, "//", 0, 2) == 0)
        return parse_url($site)['scheme'] . $path;
    elseif (
        substr_compare($path, "http://", 0, 7) == 0 or
        substr_compare($path, "https://", 0, 8) == 0 or
        substr_compare($path, "www.", 0, 4) == 0
    )
        return $path;
    else
        return $site . '/' . $path;
}

function ignoreLink($url)
{
    return $url[0] == "#" or substr($url, 0, 11) == "javascript:";
}

function insertIntoDatabase($link, $title, &$metaData, $depth)
{
    echo ("Inserting new record {URL= $link" .
        ", Title = '$title'" .
        ", Description = '" . $metaData['description'] .
        "', Keywords = ' " . $metaData['keywords'] .
        "'}<br/><br/><br/>");
    $crawledLinks[] = $link;
}

function getDocTitle(&$doc, $url)
{
    $titleNodes = $doc->getElementsByTagName('title');
    if (count($titleNodes) == 0 or !isset($titleNodes[0]->nodeValue))
        return $url;
    $title = str_replace('', '\n', $titleNodes[0]->nodeValue);
    return (strlen($title) < 1) ? $url : $title;
}

function getDocMetaData(&$doc)
{
    $metaData = array();
    $metaNodes = $doc->getElementsByTagName('meta');
    foreach ($metaNodes as $node)
        $metaData[$node->getAttribute("name")]
            = $node->getAttribute("content");
    if (!isset($metaData['description']))
        $metaData['description'] = 'No Description Available';
    if (!isset($metaData['keywords'])) $metaData['keywords'] = '';
    return array(
        'keywords' => str_replace('', '\n', $metaData['keywords']),
        'description' => str_replace('', '\n', $metaData['description'])
    );
}

followLink("http://example.com/");
