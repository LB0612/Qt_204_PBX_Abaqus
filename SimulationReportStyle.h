#ifndef SIMULATIONREPORTSTYLE_H
#define SIMULATIONREPORTSTYLE_H

namespace SimulationReportStyle
{

constexpr double PageWidthMm = 210.0;
constexpr double PageHeightMm = 297.0;

constexpr double MarginLeftMm = 30.0;
constexpr double MarginRightMm = 20.0;
constexpr double MarginTopMm = 30.0;
constexpr double MarginBottomMm = 25.0;

constexpr double HeaderMm = 25.0;
constexpr double FooterMm = 18.0;

constexpr double CoverTitlePt = 32.0;
constexpr double CoverSubtitlePt = 18.0;
constexpr double CoverInfoPt = 16.0;
constexpr double CoverDatePt = 18.0;

constexpr double Heading1Pt = 15.0;
constexpr double Heading2Pt = 12.0;
constexpr double Heading3Pt = 12.0;

constexpr double BodyPt = 12.0;
constexpr double TablePt = 10.5;
constexpr double CaptionPt = 10.5;
constexpr double HeaderPt = 12.0;
constexpr double FooterPt = 10.5;

constexpr double FigureMaxWidthMm = 150.0;
constexpr double FigureMaxHeightMm = 180.0;

constexpr double TableWidthPct = 0.86;

// Cover: page top margin + this spacer ≈ PDF title at 70 mm.
constexpr double CoverTopSpacerMm = 40.0;
constexpr double CoverAfterTitleMm = 14.0;
constexpr double CoverAfterSubtitleMm = 18.0;
constexpr double CoverBeforeInfoMm = 10.0;
constexpr double CoverInfoGapMm = 6.0;
constexpr double CoverBeforeDateMm = 18.0;

} // namespace SimulationReportStyle

#endif
