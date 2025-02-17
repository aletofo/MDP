int DiStefano(const Mat1b &img, Mat1i &imgOut) {
	imgOut = Mat1i(img.size());

	int iNewLabel(0);
	// p q r		  p
	// s x			q x
	// lp,lq,lx: labels assigned to p,q,x
	// FIRST SCAN:
	int *aClass = new int[img.rows*img.cols / 4];
	bool *aSingle = new bool[img.rows*img.cols / 4];
	for (int y = 0; y < img.rows; y++) {
		for (int x = 0; x < img.cols; x++) {
			if (img(y, x)) {

                int lp(0), lq(0), lr(0), ls(0), lx(0); // lMin(INT_MAX);
				if (y > 0) {
					if (x > 0)
						lp = imgOut(y - 1, x - 1);
					lq = imgOut(y - 1, x);
					if (x < img.cols - 1)
						lr = imgOut(y - 1, x + 1);
				}
				if (x > 0)
					ls = imgOut(y, x - 1);

				// if everything around is background
				if (lp == 0 && lq == 0 && lr == 0 && ls == 0) {
					lx = ++iNewLabel;
					aClass[lx] = lx;
					aSingle[lx] = true;
				}
				else {
					// p
					lx = lp;
					// q
					if (lx == 0)
						lx = lq;
					// r
					if (lx > 0) {
						if (lr > 0 && aClass[lx] != aClass[lr]) {
							if (aSingle[aClass[lx]]) {
								aClass[lx] = aClass[lr];
								aSingle[aClass[lr]] = false;
							}
							else if (aSingle[aClass[lr]]) {
								aClass[lr] = aClass[lx];
								aSingle[aClass[lx]] = false;
							}
							else {
								int iClass = aClass[lr];
								for (int k = 1; k <= iNewLabel; k++) {
									if (aClass[k] == iClass) {
										aClass[k] = aClass[lx];
									}
								}
							}
						}
					}
					else
						lx = lr;
					// s
					if (lx > 0) {
						if (ls > 0 && aClass[lx] != aClass[ls]) {
							if (aSingle[aClass[lx]]) {
								aClass[lx] = aClass[ls];
								aSingle[aClass[ls]] = false;
							}
							else if (aSingle[aClass[ls]]) {
								aClass[ls] = aClass[lx];
								aSingle[aClass[lx]] = false;
							}
							else {
								int iClass = aClass[ls];
								for (int k = 1; k <= iNewLabel; k++) {
									if (aClass[k] == iClass) {
										aClass[k] = aClass[lx];
									}
								}
							}
						}
					}
					else
						lx = ls;
				}

				imgOut(y, x) = lx;
			}
			else
				imgOut(y, x) = 0;
		}
	}

	// Renumbering of labels
	int *aRenum = new int[iNewLabel + 1];
	int iCurLabel = 0;
	for (int k = 1; k <= iNewLabel; k++) {
		if (aClass[k] == k) {
			iCurLabel++;
			aRenum[k] = iCurLabel;
		}
	}
	for (int k = 1; k <= iNewLabel; k++)
		aClass[k] = aRenum[aClass[k]];

	// SECOND SCAN 
	for (int y = 0; y < imgOut.rows; y++) {
		for (int x = 0; x < imgOut.cols; x++) {
			int iLabel = imgOut(y, x);
			if (iLabel > 0)
				imgOut(y, x) = aClass[iLabel];
		}
	}

	delete[] aClass;
	delete[] aSingle;
	delete[] aRenum;
	return iCurLabel + 1;
}