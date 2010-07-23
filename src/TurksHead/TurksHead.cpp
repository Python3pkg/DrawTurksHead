// Header
#include "TurksHead.hpp"

// Standard library
#include <map>
#include <iostream> /// @todo Remove
#include <cassert>

// Boost
#include <boost/math/common_factor.hpp>
#include <boost/utility.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/foreach.hpp>
#include <boost/utility.hpp>

#define foreach BOOST_FOREACH

namespace TurksHead {

TurksHead::TurksHead( int leads, int bights, double innerRadius, double outerRadius, double lineWidth ) :
    m_leads( leads ),
    m_bights( bights ),
    m_paths( boost::math::gcd( m_bights, m_leads ) ),
    m_maxThetaOnPath( 2 * m_leads * m_bights * s_stepsTheta / m_paths ),
    m_radius( ( innerRadius + outerRadius ) / 2 ),
    m_deltaRadius( ( outerRadius - innerRadius - lineWidth ) / 2 ),
    m_lineWidth( lineWidth )
{
    computeKnownAltitudes();
}

const int TurksHead::s_stepsTheta = 20;

void TurksHead::draw( Cairo::RefPtr< Cairo::Context > context ) const {
    m_ctx = context;
    m_ctx->save();

    drawPaths( false );
    drawPaths( true );

    m_ctx->restore();
}

void TurksHead::drawPaths( bool onlyPositiveZ ) const {
    for( int path = 0; path < m_paths; ++path ) {
        drawPath( path, onlyPositiveZ );
        m_ctx->rotate( 2 * M_PI / m_paths );
    }
}

void TurksHead::drawPath( int path, bool onlyPositiveZ ) const {
    for( int theta = 0; theta <= m_maxThetaOnPath; ++theta ) {
        double z = getAltitude( theta );
        if( !onlyPositiveZ || z > 0 ) {
            setSourceHsv( theta * 360. / m_maxThetaOnPath, 0.5, 0.5 + z / 2 );
            drawSegment( theta );
            m_ctx->fill();
        }
    }
}

void TurksHead::drawSegment( int theta ) const {
    double x0, y0; boost::tie( x0, y0 ) = getCoordinates( theta - 1 );
    double x1, y1; boost::tie( x1, y1 ) = getCoordinates( theta + 1 );

    double dx = x1 - x0;
    double dy = y1 - y0;
    double n = std::sqrt( dx * dx + dy * dy );

    double nx = -m_lineWidth * dy / n / 2;
    double ny = m_lineWidth * dx / n / 2;

    m_ctx->move_to( x0 + nx, y0 + ny );
    m_ctx->line_to( x1 + nx, y1 + ny );
    m_ctx->line_to( x1 - nx, y1 - ny );
    m_ctx->line_to( x0 - nx, y0 - ny );
    m_ctx->close_path();
}

double TurksHead::getAltitude( int theta ) const {
    std::map< int, int >::const_iterator nextIt = m_knownAltitudes.lower_bound( theta );
    assert( nextIt != m_knownAltitudes.begin() );
    assert( nextIt != m_knownAltitudes.end() );
    std::map< int, int >::const_iterator prevIt = boost::prior( nextIt );
    return prevIt->second + ( nextIt->second - prevIt->second ) * ( theta - prevIt->first ) / float( nextIt->first - prevIt->first );
}

void TurksHead::computeKnownAltitudes() {
    int alt = -1;
    for( int i = -1; i <= 2 * m_leads * m_bights + 1; ++i ) {
        if( i % m_leads ) {
            double angle = i * s_stepsTheta;
            m_knownAltitudes[ angle ] = alt;
            alt *= -1;
        }
    }
}

boost::tuple< double, double > TurksHead::getCoordinates( int theta ) const {
    return convertRadialToCartesianCoordinates( getRadius( theta ), theta );
}

double TurksHead::getRadius( int theta ) const {
    return m_radius + m_deltaRadius * cos( m_bights * angleFromTheta( theta ) / m_leads );
}

boost::tuple< double, double > TurksHead::convertRadialToCartesianCoordinates( double radius, int theta ) const {
    return boost::make_tuple(
        radius * std::cos( angleFromTheta( theta ) ),
        radius * std::sin( angleFromTheta( theta ) )
    );
}

double TurksHead::angleFromTheta( int theta ) const {
    // Theta evolves in [ 0, 2 * m_leads * m_bights * s_stepsTheta / m_paths ]
    // Angle evolves in [ 0, 2 * M_PI * m_leads / m_paths ]
    return M_PI * theta / m_bights / s_stepsTheta;
}

void TurksHead::setSourceHsv( double h, double s, double v ) const {
    int hi = h / 60;
    double f = h / 60 - hi;
    hi %= 60;
    double p = v * ( 1 - s );
    double q = v * ( 1 - f * s );
    double t = v * ( 1 - ( 1 - f ) * s );
    switch( hi ) {
        case 0: return m_ctx->set_source_rgb( v, t, p );
        case 1: return m_ctx->set_source_rgb( q, v, p );
        case 2: return m_ctx->set_source_rgb( p, v, t );
        case 3: return m_ctx->set_source_rgb( p, q, v );
        case 4: return m_ctx->set_source_rgb( t, p, v );
        case 5: return m_ctx->set_source_rgb( v, p, q );
    }
}

} // Namespace
