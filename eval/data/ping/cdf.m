function cdf_data = cdf(filename, color)
	%raw_data = load filename;
	%sort_data = sort(raw_data);

    sort_data = sort(filename);

	cdf_data = sort_data*[1,0];

	[tate, yoko] = size(cdf_data);

	for n=1:1:tate
		cdf_data(n,2) = int32(n*100/tate);
	end

	plot(cdf_data(:,1),cdf_data(:,2), color);
end
