function changeSite() {
    website = document.getElementById("siteList").value;
}

function changeCsv() {
    CSVFile = document.getElementById("batchList").value;
}

function startup() {
    changeSite();
    changeCsv();
    drawGraph();
}

window.onload = startup();

function drawGraph() {
    //d3.selectAll("*").remove();
    d3.select("svg").remove();
    var dataObj = {};
    /*
        dataObj is a multi level dictionary in which the first level
        contains websites, and each value associated with a website
        is a dictionary of search terms -> number of occurences
    */
    var margin = {top: 20, right: 10, bottom: 100, left: 50};
    var width = 700 - margin.left - margin.right;
    var height = 500 - margin.top - margin.bottom;

    var svg = d3.select("#canvas")
                .append("svg")
                    .attr("height", height + margin.top + margin.bottom)
                    .attr("width", width + margin.left + margin.right)
                .append("g")
                    .attr("transform", "translate(" + margin.left + "," + margin.right + ")");

    var xScale = d3.scalePoint()
                .padding(0.5)
                .range([0,width]);

    var yScale = d3.scaleLinear()
                .range([height, 0]);

    // x and y axis
    var xAxis = d3.axisBottom()
                .scale(xScale);

    var yAxis = d3.axisLeft()
                .scale(yScale);

    d3.csv(CSVFile, function(error, data) {
        if(error) throw error;
        // need to aggregate data
        data.forEach(function(d) {
            dataObj["time"] = d["Time"];
            if (!(d.Site in dataObj)) {
                dataObj[d.Site] = {};
            }
            if (d.Phrase in dataObj) {
                dataObj[d.Site][d.Phrase] = dataObj[d.Site][d.Phrase] + d.Count;
            } else {
                dataObj[d.Site][d.Phrase] = d.Count;
            }
        });
        siteData = {};
        // replace with user choice eventually
        if(website in dataObj) {
            siteData = dataObj[website];
        } else {
            alert("No data for " + website + " in file " + CSVFile);
        }
        // change title based on time of batch
        d3.select("h1").text("Site Stats for " + dataObj["time"]);
        var keys = Object.keys(siteData);
        var values = Object.values(siteData);
        // turn values into integers
        for(i in values) {
            values[i] = +values[i];
        }

        xScale.domain(keys.map(function(k) { return k; }));
        yScale.domain([0, d3.max(values)]);

        svg.selectAll("rect")
            .data(values)
            .enter().append("rect")
            .attr("height", 0)
            .attr("y", height)
            .transition().duration(2000)
            .delay(function(d, i) { return i * 200; })
            .attr("x", function(d, i) { return xScale(keys[i]); })
            .attr("y", function(d) { return yScale(d); })
            .attr("width", "40")
            .attr("height", function(d) { return height - yScale(d); })
            .attr("class","bar")
            .style("fill", "blue");

        // Draw xAxis and position the label
        svg.append("g")
            .attr("class", "x axis")
            .attr("transform", "translate(0," + height + ")")
            .call(xAxis)
            .selectAll("text")
            .attr("dx", "-.8em")
            .attr("dy", ".25em")
            .attr("transform", "rotate(-60)" )
            .style("text-anchor", "end")
            .attr("font-size", "10px");


        // Draw yAxis and postion the label
        svg.append("g")
            .attr("class", "y axis")
            .call(yAxis)
            .append("text")
            .attr("transform", "rotate(-90)")
            .attr("x", -height/2)
            .attr("dy", "3em")
            .style("text-anchor", "middle")
            .text("Number of Occurences");

    });
}